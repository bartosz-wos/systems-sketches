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
};
