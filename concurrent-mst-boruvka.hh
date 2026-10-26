#pragma once

namespace graph{

struct edge{
  uint32_t u;
  uint32_t v;
  uint32_t weight;
  uint32_t id;
};

struct mst_result{
  uint64_t tot_weight = 0;
  uint32_t comps = 0;
  std::vector<uint32_t> mst_edge_ids;
};

class ConcurrentBoruvka{
  static constexpr uint64_t INF = std::numeric_limits<uint64_t>::max();

  static mst_result get_mst(uint32_t vertices, const std::vector<edge>& edges, uint32_t threads = std::thread::hardware_concurrency()){
    if(vertices <= 1 || edges.empty()) [[unlikely]]{
      return { 0, vertices, {} };
    }

    if(threads == 0) [[unlikely]]{
      threads = 1;
    }

    threads = std::min<uint32_t>(threads, static_cast<uint32_t>(edges.size())); // edges.size() > 0, no need for == 0 case
    
    auto dsu = std::make_unique<std::atomic<uint32_t>[]>(vertices);
    auto best_edges = std::make_unique<std::atomic<uint32_t>[]>(vertices);

    struct thread_local_state{
      std::vector<uint32_t> local_mst_edges{};
      uint64_t local_weight = 0;
      uint32_t merges_this_phase = 0;
    };

    std::vector<thread_local_state> thread_states(vertices);

    std::atomic<uint32_t> remaining_comps{ vertices };
    std::atomic<uint32_t> merges{ 0 };
    bool stop = false;


    // should be fine without reader/writer locks
    // path compression by halving the path, so we connected node v to a node that is a parent of a parent
    auto find_root = [&](uint32_t v, std::memory_order order) noexcept -> uint32_t{
      uint32_t cur = v;

      while(true){
        uint32_t p = dsu[cur].load(order);
        if(p == cur){
          return cur;
        }

        uint32_t par = dsu[p].load(order);
        if(par != p){
          dsu[cur].store(par, std::memory_order_relaxed);
        }

        cur = par;
      }
    };

    // attach bigger root id to the smaller one
    auto set_union = [&](uint32_t u, uint32_t v) noexcept -> uint32_t{
      while(true){
        uint32_t pu = find_root(u, std::memory_order_acquire);
        uint32_t pv = find_root(v, std::memory_order_acquire);

        if(pu == pv){
          return false;
        }

        if(pu > pv){
          std::swap(pu, pv);
        }

        uint32_t expected = pv;
        if(dsu[pv].compare_exchange_weak(expected, pu, std::memory_order_release, std::memory_order_relaxed)){
          return true;
        }
      }
    };

  }
};

}
