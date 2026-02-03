#include "kvstore/kvstore.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace kv {

// ---------- Constructors ----------
KVStore::KVStore() {
  // in-memory mode: values are cached in Entry
}

KVStore::KVStore(const std::string& log_path)
    : persistence_enabled_(true), log_path_(log_path) {
  ReplayLog();
}

// ---------- Public API ----------
bool KVStore::Put(const std::string& key, const std::string& value) {
  if (!persistence_enabled_) {
    Entry e;
    e.in_memory = true;
    e.cached = value;
    index_[key] = std::move(e);
    return true;
  }

  uint64_t value_offset = 0;
  if (!AppendPut(key, value, &value_offset)) return false;

  Entry e;
  e.offset = value_offset;
  e.size = static_cast<uint64_t>(value.size());
  index_[key] = std::move(e);
  return true;
}

std::optional<std::string> KVStore::Get(const std::string& key) const {
  auto it = index_.find(key);
  if (it == index_.end()) return std::nullopt;

  const Entry& e = it->second;
  if (!persistence_enabled_ || e.in_memory) {
    return e.cached;
  }
  return ReadValueAt(e.offset, e.size);
}

bool KVStore::Del(const std::string& key) {
  bool existed = (index_.erase(key) > 0);
  if (persistence_enabled_) {
    AppendDel(key);  // log deletes even if key missing
  }
  return existed;
}

void KVStore::Close() {
  // No persistent handle kept open yet.
}

// ---------- Persistence helpers ----------
static void WriteLine(std::ofstream& out, const std::string& s) {
  out.write(s.data(), static_cast<std::streamsize>(s.size()));
  out.put('\n');
}

bool KVStore::AppendPut(const std::string& key, const std::string& value, uint64_t* value_offset_out) {
  std::ofstream out(log_path_, std::ios::binary | std::ios::app);
  if (!out) return false;

  // where are we before writing header?
  std::streampos record_start = out.tellp();

  std::string header = "PUT " + key + " " + std::to_string(value.size());
  WriteLine(out, header);

  // value bytes begin right after "header\n"
  std::streampos value_pos = out.tellp();
  *value_offset_out = static_cast<uint64_t>(value_pos);

  out.write(value.data(), static_cast<std::streamsize>(value.size()));
  out.put('\n');
  out.flush();

  (void)record_start;
  return static_cast<bool>(out);
}

bool KVStore::AppendDel(const std::string& key) {
  std::ofstream out(log_path_, std::ios::binary | std::ios::app);
  if (!out) return false;

  WriteLine(out, "DEL " + key);
  out.flush();
  return static_cast<bool>(out);
}

std::optional<std::string> KVStore::ReadValueAt(uint64_t offset, uint64_t size) const {
  std::ifstream in(log_path_, std::ios::binary);
  if (!in) return std::nullopt;

  in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  if (!in) return std::nullopt;

  std::string value;
  value.resize(size);
  in.read(&value[0], static_cast<std::streamsize>(size));
  if (in.gcount() != static_cast<std::streamsize>(size)) return std::nullopt;

  return value;
}

void KVStore::ReplayLog() {
  index_.clear();

  std::ifstream in(log_path_, std::ios::binary);
  if (!in) return;  // no file yet

  std::string header;
  while (std::getline(in, header)) {
    if (header.empty()) continue;

    std::istringstream iss(header);
    std::string op;
    iss >> op;

    if (op == "PUT") {
      std::string key;
      uint64_t value_size = 0;
      iss >> key >> value_size;
      if (key.empty()) throw std::runtime_error("Bad PUT header in log");

      // value bytes begin at current position
      std::streampos value_pos = in.tellg();
      if (value_pos == std::streampos(-1)) break;

      // Skip over the value bytes without allocating
      in.seekg(static_cast<std::streamoff>(value_size), std::ios::cur);
      if (!in) break;

      // Consume trailing newline after value
      char nl = 0;
      in.get(nl);
      if (!in || nl != '\n') {
        // truncated/corrupt record -> stop replay safely
        break;
      }

      Entry e;
      e.offset = static_cast<uint64_t>(value_pos);
      e.size = value_size;
      index_[key] = std::move(e);

    } else if (op == "DEL") {
      std::string key;
      iss >> key;
      if (key.empty()) throw std::runtime_error("Bad DEL header in log");
      index_.erase(key);

    } else {
      // unknown op -> stop safely
      break;
    }
  }
}

// ---------- Compaction ----------
bool KVStore::Compact() {
  if (!persistence_enabled_) return true;

  namespace fs = std::filesystem;
  auto parent = fs::path(log_path_).parent_path();
  if (!parent.empty()) {
    fs::create_directories(parent);
  }


  const std::string tmp = log_path_ + ".tmp";
  const std::string bak = log_path_ + ".bak";

  // Write a brand-new compacted log containing only latest live keys
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) return false;

    for (const auto& [key, entry] : index_) {
      auto v = Get(key);
      if (!v) continue;

      std::string header = "PUT " + key + " " + std::to_string(v->size());
      out.write(header.data(), static_cast<std::streamsize>(header.size()));
      out.put('\n');
      out.write(v->data(), static_cast<std::streamsize>(v->size()));
      out.put('\n');
    }
    out.flush();
    if (!out) return false;
  }

  // Replace old log atomically-ish (safe enough for this stage)
  try {
    if (fs::exists(bak)) fs::remove(bak);
    if (fs::exists(log_path_)) fs::rename(log_path_, bak);
    fs::rename(tmp, log_path_);
    if (fs::exists(bak)) fs::remove(bak);
  } catch (...) {
    return false;
  }

  // Rebuild index from the compacted log
  ReplayLog();
  return true;
}

}  // namespace kv
