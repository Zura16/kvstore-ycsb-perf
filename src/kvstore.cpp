#include "kvstore/kvstore.h"

namespace kv {

bool KVStore::Put(const std::string& key, const std::string& value) {
  map_[key] = value;
  return true;
}

std::optional<std::string> KVStore::Get(const std::string& key) const {
  auto it = map_.find(key);
  if (it == map_.end()) return std::nullopt;
  return it->second;
}

bool KVStore::Del(const std::string& key) {
  return map_.erase(key) > 0;
}

}  // namespace kv
