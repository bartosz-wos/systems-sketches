#pragma once

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstdint>
#include <limits>
#include <memory>
#include <thread>
#include <vector>

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
public:
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
    auto best_edges = std::make_unique<std::atomic<uint64_t>[]>(vertices);

    struct thread_local_state{
      std::vector<uint32_t> local_mst_edges{};
      uint64_t local_weight = 0;
      uint32_t merges_this_phase = 0;
    };

    std::vector<thread_local_state> thread_states(threads);

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

    // cas loop for updating the chosen edge for a component
    auto update_best_edge = [&](uint32_t comp, uint64_t cost_id) noexcept{
      uint64_t cur = best_edges[comp].load(std::memory_order_relaxed);
      while(cost_id < cur){
        if(best_edges[comp].compare_exchange_weak(cur, cost_id, std::memory_order_relaxed, std::memory_order_relaxed)){
          break;
        }
      }
    };

    std::barrier phase_barrier(threads);

    // the function runs when barrier falls
    std::barrier round_finish_barrier(threads, [&]() noexcept{
        uint32_t merged = merges.load(std::memory_order_relaxed);
        uint32_t rem = remaining_comps.fetch_sub(merged, std::memory_order_relaxed) - merged;

        if(merged == 0 || rem <= 1){
          stop = true;
        }

        merges.store(0, std::memory_order_relaxed);
    });


    auto mst_run = [&](uint32_t thread_id){
      const uint32_t v_block = (vertices + threads - 1) / threads;
      const uint32_t v_begin = std::min<uint32_t>(vertices, thread_id * v_block);
      const uint32_t v_end = std::min<uint32_t>(vertices, v_begin + v_block);

      const std::size_t num_edges = edges.size();
      const std::size_t e_chunk = (num_edges + threads - 1) / threads;
      const std::size_t e_begin = std::min<std::size_t>(num_edges, static_cast<std::size_t>(thread_id) * e_chunk);
      const std::size_t e_end = std::min<std::size_t>(num_edges, e_begin + e_chunk);

      for(uint32_t v = v_begin; v < v_end; ++v){
        dsu[v].store(v, std::memory_order_relaxed);
        best_edges[v].store(INF, std::memory_order_relaxed);
      }
      thread_states[thread_id].local_mst_edges.reserve(vertices / threads + 16);

      phase_barrier.arrive_and_wait();

      while(!stop){
        for(std::size_t i = e_begin; i < e_end; ++i){
          uint32_t pu = find_root(edges[i].u, std::memory_order_relaxed);
          uint32_t pv = find_root(edges[i].v, std::memory_order_relaxed);

          if(pu != pv){
            uint64_t cost_id = (static_cast<uint64_t>(edges[i].weight) << 32) | edges[i].id;
            update_best_edge(pu, cost_id);
            update_best_edge(pv, cost_id);
          }
        }

        phase_barrier.arrive_and_wait();

        uint32_t local_merges = 0;

        for(uint32_t v = v_begin; v < v_end; ++v){
          uint64_t cost_id = best_edges[v].load(std::memory_order_relaxed);
          if(cost_id == INF){
            continue;
          }

          best_edges[v].store(INF, std::memory_order_relaxed);

          uint32_t edges_idx = static_cast<uint32_t>(cost_id & 0xFFFFFFFFULL);
          const edge& chosen_edge = edges[edges_idx];

          if(set_union(chosen_edge.u, chosen_edge.v)){
            thread_states[thread_id].local_mst_edges.push_back(chosen_edge.id);
            thread_states.local_weight += chosen_edge.weight;
            ++local_merges;
          }
        }

        if(local_merges > 0){
          merges.fetch_add(local_merges, std::memory_order_relaxed);
        }

        phase_barrier.arrive_and_wait();

        for(uint32_t v = v_begin; v < v_end; ++v){
          find_root(v, std::memory_order_relaxed);
        }

        round_finish_barrier.arrive_and_wait();
      }
    };

    std::vector<std::thread> threads_vector;
    threads_vector.reserve(threads - 1);

    for(uint32_t i = 1; i < threads; ++i){
      threads_vector.emplace_back(mst_run, i);
    }

    mst_run(0);

    for(auto& t : threads_vector){
      if(t.joinable()){
        t.join();
      }
    }

    mst_result result;
    result.comps = remaining_comps.load(std::memory_order_relaxed);
    result.mst_edge_ids.reserve(vertices - result.comps);

    for(uint32_t i = 0; i < threads; ++i){
      result.tot_weight += thread_local_state[i].local_weight;
      result.mst_edge_ids.insert(
          result.mst_edge_ids.end(),
          thread_local_state[i].local_mst_edges.begin(),
          thread_local_state[i].local_mst_edges.end()
      );
    }

    return result;
  }
};

} // namespace graph
