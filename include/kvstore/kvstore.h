#pragma once
#include <optional>
#include <string>
#include <unordered_map>

namespace kv {

class KVStore {
 public:
  // In-memory store (no persistence)
  KVStore();

  // Persistent store using an append-only log file
  explicit KVStore(const std::string& log_path);

  bool Put(const std::string& key, const std::string& value);
  std::optional<std::string> Get(const std::string& key) const;
  bool Del(const std::string& key);

  // Flush/close file handle if persistence enabled
  void Close();

 private:
  bool persistence_enabled_ = false;
  std::string log_path_;

  std::unordered_map<std::string, std::string> map_;

  bool AppendPut(const std::string& key, const std::string& value);
  bool AppendDel(const std::string& key);
  void ReplayLog();
};

}  // namespace kv
