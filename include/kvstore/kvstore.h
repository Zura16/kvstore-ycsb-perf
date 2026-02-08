#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <shared_mutex>



namespace kv {

struct Entry {
  uint64_t offset = 0;  // where value bytes begin in the log file
  uint64_t size = 0;    // number of bytes in the value
  bool in_memory = false;
  std::string cached;   // optional cache (used for non-persistent mode)
};

class KVStore {
 public:
  KVStore();
  explicit KVStore(const std::string& log_path);

  bool Put(const std::string& key, const std::string& value);
  std::optional<std::string> Get(const std::string& key) const;
  bool Del(const std::string& key);

  // Week 3:
  bool Compact();  // rewrite log to keep only latest live keys

  void Close();

 private:
  bool persistence_enabled_ = false;
  std::string log_path_;
  mutable std::shared_mutex mu_;
  std::unordered_map<std::string, Entry> index_;

  // persistence
  bool AppendPut(const std::string& key, const std::string& value, uint64_t* value_offset_out);
  bool AppendDel(const std::string& key);

  void ReplayLog();
  std::optional<std::string> ReadValueAt(uint64_t offset, uint64_t size) const;
};

}  // namespace kv
