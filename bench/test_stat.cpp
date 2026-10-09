#include "profiler.hh"

#include <algorithm>
#include <cassert>
#include <pthread.h>
#include <ranges>
#include <vector>
#include <cstddef>
#include <sched.h>
#include <thread>

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
    .p99 = samples[static_cast<std::size_t>(iters * 0.99)],
    .max = samples.back()
  };
}

void pin_thread(unsigned int core_id){
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void print_stats(std::string_view name, const Stats& s, std::string_view unit){
#if defined(__cpp_lib_print)
  std::println("{:<32} | min: {:>4} | p50: {:>4} | p99 {:>5} | max: {:>6} {}", name, s.min, s.p50, s.p99, s.max, unit);
#else
  std::cout << std::setw(32) << name
            << " | min: " << std::setw(4) << s.min
            << " | p50: " << std::setw(4) << s.p50
            << " | p99: " << std::setw(5) << s.p99
            << " | max: " << std::setw(6) << s.max
            << " " << unit << '\n';
#endif
}

int main(){
  int temp = 0;
  for(int i{ 0 }; i < 50'000'000; ++i){
    bench::do_not_optimize(temp += i);
  }

  pin_thread(0);

#if defined(__cpp_lib_print)
  std::println("Clock overhead with isolated core");
#else
  std::cout << "Clock overhead with isolated core\n";
#endif

  auto stat_chrono = measure_clocks_overhead<bench::ChronoClock>();
  print_stats("ChronoClock", stat_chrono, bench::ChronoClock::unit);

  auto stat_rdtsc = measure_clocks_overhead<bench::RdtscClock>();
  print_stats("raw RdtscClock", stat_rdtsc, bench::RdtscClock::unit);

  auto stat_rdtscp = measure_clocks_overhead<bench::RdtscpClock>();
  print_stats("raw RdtscpClock", stat_rdtscp, bench::RdtscpClock::unit);

  auto stat_ser = measure_clocks_overhead<bench::SerializedRdtscClock<false>>();
  print_stats("SerializedRdtscClock without core tracking", stat_ser, bench::SerializedRdtscClock<false>::unit);

  auto stat_ser_tracked = measure_clocks_overhead<bench::SerializedRdtscClock<true>>();
  print_stats("SerializedRdtscClock with core tracking", stat_ser_tracked, bench::SerializedRdtscClock<true>::unit);

  unsigned int cores_available = std::thread::hardware_concurrency();


}
