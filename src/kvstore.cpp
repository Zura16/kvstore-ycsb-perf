#include "kvstore/kvstore.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace kv {

// ---------- Constructors ----------
KVStore::KVStore() = default;

KVStore::KVStore(const std::string& log_path)
    : persistence_enabled_(true), log_path_(log_path) {
  ReplayLog();
}

// ---------- Public API ----------
bool KVStore::Put(const std::string& key, const std::string& value) {
  map_[key] = value;
  if (persistence_enabled_) return AppendPut(key, value);
  return true;
}

std::optional<std::string> KVStore::Get(const std::string& key) const {
  auto it = map_.find(key);
  if (it == map_.end()) return std::nullopt;
  return it->second;
}

bool KVStore::Del(const std::string& key) {
  bool existed = (map_.erase(key) > 0);
  if (persistence_enabled_) {
    // We still log DEL even if key doesn't exist (keeps log semantics simple).
    AppendDel(key);
  }
  return existed;
}

void KVStore::Close() {
  // No persistent stream held open yet (we open per-append for simplicity).
  // Later we can keep a stream open for performance.
}

// ---------- Persistence helpers ----------
static void WriteLine(std::ofstream& out, const std::string& s) {
  out.write(s.data(), static_cast<std::streamsize>(s.size()));
  out.put('\n');
}

bool KVStore::AppendPut(const std::string& key, const std::string& value) {
  std::ofstream out(log_path_, std::ios::binary | std::ios::app);
  if (!out) return false;

  // Format:
  // PUT <key> <value_size>\n
  // <value_bytes>\n
  WriteLine(out, "PUT " + key + " " + std::to_string(value.size()));
  out.write(value.data(), static_cast<std::streamsize>(value.size()));
  out.put('\n');
  out.flush();
  return static_cast<bool>(out);
}

bool KVStore::AppendDel(const std::string& key) {
  std::ofstream out(log_path_, std::ios::binary | std::ios::app);
  if (!out) return false;

  // Format: DEL <key>\n
  WriteLine(out, "DEL " + key);
  out.flush();
  return static_cast<bool>(out);
}

void KVStore::ReplayLog() {
  std::ifstream in(log_path_, std::ios::binary);
  if (!in) {
    // File doesn't exist yet — that's fine.
    return;
  }

  std::string header;
  while (std::getline(in, header)) {
    if (header.empty()) continue;

    std::istringstream iss(header);
    std::string op;
    iss >> op;

    if (op == "PUT") {
      std::string key;
      size_t value_size = 0;
      iss >> key >> value_size;

      if (key.empty()) throw std::runtime_error("Bad PUT header in log");

      std::string value;
      value.resize(value_size);

      // Read exact bytes
      in.read(&value[0], static_cast<std::streamsize>(value_size));
      if (in.gcount() != static_cast<std::streamsize>(value_size)) {
        // Truncated record (crash mid-write) -> stop replay
        break;
      }

      // Consume trailing newline after the value
      char nl = 0;
      in.get(nl);
      if (!in || nl != '\n') {
        // Corrupt/truncated -> stop replay
        break;
      }

      map_[key] = value;

    } else if (op == "DEL") {
      std::string key;
      iss >> key;
      if (key.empty()) throw std::runtime_error("Bad DEL header in log");
      map_.erase(key);

    } else {
      // Unknown op -> stop replay (safer than applying garbage)
      break;
    }
  }
}

}  // namespace kv
