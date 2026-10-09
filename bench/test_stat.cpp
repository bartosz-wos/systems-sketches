#include "profiler.hh"



#if __has_include(<print>)
# include <print>
#endif

#if !defined(__cpp_lib_print)
# include <iomanip>
# include <iostream>
#endif

static_assert(sizeof(bench::Point<uint64_t, bench::EmptyCoreTracker>) == 8);
static_assert(sizeof(bench::Point<uint64_t, bench::CoreTracker>) == 16);


struct Stats{
  uint64_t min;
  uint64_t p50;
  uint64_t p99;
  uint64_t max;
};

template<bench::Clock C>
Stats measure_clocks_overhead(std::size_t iters = 100'000){
  assert(iters > 0);
  std::vector<uint64_t> samples;
  samples.reserve(iters);

  for(std::size_t i{ 0 }; i < iters; ++i){
    auto t0 = C::start();
    auto t1 = C::stop();
    samples.push_back(C::elapsed(t0, t1));
  }

  std::ranges::sort(samples);

  return Stats{
    .min = samples.front(),
    .p50 = samples[iters >> 1],
    .p99 = samples[statc_cast<std::size_t>(iters * 0.99)],
    .max = samples.back();
  };
}

void pin_thread(unsigned int core_id){
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);
  pthread_setaffinity(pin_thread_self(), sizeof(cpu_set_t), &cpuset);
}


int main(){

}
