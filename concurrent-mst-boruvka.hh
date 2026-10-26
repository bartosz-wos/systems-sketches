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


  }
};

}
