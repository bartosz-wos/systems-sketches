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

};

}
