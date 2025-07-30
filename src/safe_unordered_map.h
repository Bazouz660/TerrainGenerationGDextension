#ifndef SAFE_UNORDERED_MAP
#define SAFE_UNORDERED_MAP

#include <unordered_map>
#include <mutex>
#include <optional>

// A threadsafe unordered_map.
template <class Key, class Value, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class SafeUnorderedMap
{
public:
  // Default constructor
  SafeUnorderedMap()
    : map()
    , m()
  {}

  // Constructor with custom hash function and/or equality function
  SafeUnorderedMap(const Hash& hash, const KeyEqual& equal = KeyEqual())
    : map(0, hash, equal)
    , m()
  {}

  // Constructor with initial bucket count and custom hash function
  SafeUnorderedMap(size_t bucket_count, const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual())
    : map(bucket_count, hash, equal)
    , m()
  {}

  ~SafeUnorderedMap(void)
  {}

  // Insert or update a key-value pair
  void insert_or_assign(const Key& key, const Value& value)
  {
    std::lock_guard<std::mutex> lock(m);
    map[key] = value;
  }

  // Insert a key-value pair only if key doesn't exist
  // Returns true if inserted, false if key already exists
  bool insert(const Key& key, const Value& value)
  {
    std::lock_guard<std::mutex> lock(m);
    auto result = map.insert({key, value});
    return result.second;
  }

  // Get a value by key
  // Returns std::nullopt if key doesn't exist
  std::optional<Value> get(const Key& key) const
  {
    std::lock_guard<std::mutex> lock(m);
    auto it = map.find(key);
    if (it != map.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  // Check if a key exists
  bool contains(const Key& key) const
  {
    std::lock_guard<std::mutex> lock(m);
    return map.find(key) != map.end();
  }

  // Remove a key-value pair
  // Returns true if removed, false if key didn't exist
  bool erase(const Key& key)
  {
    std::lock_guard<std::mutex> lock(m);
    return map.erase(key) > 0;
  }

  // Iterator-based erase for range-based operations
  // Note: This breaks thread safety for the iterator itself
  // Should only be used when you have exclusive access to the map
  typename std::unordered_map<Key, Value, Hash, KeyEqual>::iterator erase(typename std::unordered_map<Key, Value, Hash, KeyEqual>::iterator it)
  {
    std::lock_guard<std::mutex> lock(m);
    return map.erase(it);
  }

  // Get the size of the map
  size_t size() const
  {
    std::lock_guard<std::mutex> lock(m);
    return map.size();
  }

  // Check if the map is empty
  bool empty() const
  {
    std::lock_guard<std::mutex> lock(m);
    return map.empty();
  }

  // Clear all elements
  void clear()
  {
    std::lock_guard<std::mutex> lock(m);
    map.clear();
  }

  // Get all keys (snapshot at the time of call)
  std::vector<Key> keys() const
  {
    std::lock_guard<std::mutex> lock(m);
    std::vector<Key> result;
    result.reserve(map.size());
    for (const auto& pair : map) {
      result.push_back(pair.first);
    }
    return result;
  }

  // Get all values (snapshot at the time of call)
  std::vector<Value> values() const
  {
    std::lock_guard<std::mutex> lock(m);
    std::vector<Value> result;
    result.reserve(map.size());
    for (const auto& pair : map) {
      result.push_back(pair.second);
    }
    return result;
  }

  // Execute a function on a value if key exists
  // This allows atomic read-modify operations
  template<typename Func>
  bool modify(const Key& key, Func func)
  {
    std::lock_guard<std::mutex> lock(m);
    auto it = map.find(key);
    if (it != map.end()) {
      func(it->second);
      return true;
    }
    return false;
  }

    // get the key associated with a value
    std::optional<Key> get_key(const Value& value) const
    {
        std::lock_guard<std::mutex> lock(m);
        for (const auto& pair : map) {
        if (pair.second == value) {
            return pair.first;
        }
        }
        return std::nullopt;
    }


private:
  std::unordered_map<Key, Value, Hash, KeyEqual> map;
  mutable std::mutex m;
};

#endif