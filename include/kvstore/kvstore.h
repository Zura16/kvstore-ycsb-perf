#pragma once
#include <optional>
#include <string>
#include <unordered_map>

namespace kv {

class KVStore {
 public:
  bool Put(const std::string& key, const std::string& value);
  std::optional<std::string> Get(const std::string& key) const;
  bool Del(const std::string& key);

 private:
  std::unordered_map<std::string, std::string> map_;
};

}  // namespace kv
