#pragma once

template<typename Key, typename Value, typename Hash = std::hash<Key>>
class LinearProbingHashMap{
private:
  struct Node{
    Key key{};
    Value value{};
    bool used{ false };
  };

  std::vector<Node> table_;
  std::size_t size_{ 0 };
  float load_factor_{ 0.5f };
  [[no_unique_address]] Hash hash_{};

  [[nodiscard]]
  std::size_t hash_index(const Key& key, std::size_t cap) const{
    return hash_(key) % cap;
  }

  void rehash(std::size_t new_cap){
    std::vector<Node> old_table = std::move(table_);
    table_.assign(new_cap, Node{});
    size_ = 0;

    for(auto& node : old_table){
      if(node.used){
        insert(std::move(node.key), std::move(node.value));
      }
    }
  }

public:
  explicit LinearProbingHashMap(std::size_t init_capacity = 16, float load_factor = 0.5f, const Hash& hash = Hash{})
  : table_( std::max<std::size_t>(init_capacity, 8) )
  , load_factor_{ load_factor }
  , hash_{ hash }
  {}

  ~LinearProbingHashMap() = default;

  template<typename K, typename V>
  requires std::constructible_from<Key, K> && std::constructible_from<Value, V>
  bool insert(K&& key, V&& value){
    if(table_.empty() || static_cast<float>(size_ + 1) > static_cast<float>(table_.size()) * load_factor_){
      rehash(table_.empty() ? 16 : table_.size() * 2);
    }

    Key converted_key = std::forward<K>(key);

    const std::size_t cap = table_.size();
    std::size_t idx = hash_index(converted_key, cap);

    while(table_[idx].used){
      if(table_[idx].key == converted_key){
        table_[idx].value = std::forward<V>(value);
        return false;
      }

      idx = (idx + 1 >= cap) ? 0 : idx + 1;
    }

    table_[idx].key = std::move(converted_key);
    table_[idx].value = std::forward<V>(value);
    table_[idx].used = true;
    ++size_;
    return true;
  }

  [[nodiscard]]
  const Value* find(const Key& key) const noexcept{
    if(table_.empty()){
      return nullptr;
    }

    const std::size_t cap = table_.size();
    std::size_t idx = hash(key, cap);
    const std::size_t start = idx;

    while(table_[idx].used){
      if(table_[idx].key == key){
        return &table_[idx].value;
      }

      idx = (idx + 1 >= cap) ? 0 : idx + 1;
      if(idx == start){
        break;
      }
    }

    return nullptr;
  }

  [[nodiscard]]
  Value* find(const Key& key) noexcept{
    return const_cast<Value*>(std::as_const(*this).find(key));
  }

  [[nodiscard]]
  bool contains(const Key& key) const noexcept{
    return find(key) != nullptr;
  }

  bool erase(const Key& key){
    if(table_.empty()){
      return false;
    }

    const std::size_t cap = table_.size();
    std::size_t idx = hash_index(key, cap);
    const std::size_t start = idx;

    while(table_[idx].used){
      if(table_[idx].key == key){
        break;
      }

      idx = (idx + 1 >= cap) ? 0 : idx + 1;
      if(idx == start){
        break;
      }
    }

    if(!table_[idx].used || table_[idx].key != key){
      return false;
    }

    table_[idx].used = false;
    table_[idx].key = Key{};
    table_[idx].value = Value{};
    --size_;

    std::size_t nxt = (idx + 1 >= cap) ? 0 : idx + 1;
    while(table_[nxt].used){
      std::size_t h = hash_index(table_[nxt].key, cap);

      std::size_t hole_dist = (idx >= h) ? (idx - h) : (idx + cap - h);
      std::size_t nxt_dist = (nxt >= h) ? (nxt - h) : (nxt + cap - h);

      if(hole_dist < nxt_dist){
        table_[idx] = std::move(table_[nxt]);
        table_[nxt].used = false;
        table_[nxt].key = Key{};
        table_[nxt].value = Value{};
        idx = nxt;
      }

      nxt = (nxt + 1 >= cap) ? 0 : nxt + 1;
    }

    return true;
  }

};
