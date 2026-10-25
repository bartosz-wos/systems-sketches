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
};
