#include "profiler.hh"

#include <algorithm>
#include <cassert>
#include <pthread.h>
#include <ranges>
#include <vector>
#include <cstddef>
#include <sched.h>
#include <thread>
#include <random>
#include <utility>

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

static void pin_thread(unsigned int core_id){
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);
  [[maybe_unused]] int code = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  assert(code == 0 && "pthread_setaffinity_np failed");
}

template<bench::Clock C>
static Stats measure_clocks_overhead(std::size_t iters = 1u << 17){
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

static Stats measure_migration_latency(unsigned int ncores, std::size_t iters = 1u << 10){
  assert(ncores > 1);
  assert(iters > 0);

  std::mt19937 rng{ 134561 };
  std::uniform_int_distribution<unsigned int> cdist(0, ncores - 1);
  std::uniform_int_distribution<unsigned int> offdist(1, ncores - 1);

  std::vector<std::pair<unsigned int, unsigned int>> pairs;
  pairs.reserve(iters);
  for(std::size_t i{ 0 }; i < iters; ++i){
    unsigned int core = cdist(rng);
    pairs.emplace_back(core, (core + offdist(rng)) % ncores);
  }

  std::vector<uint64_t> samples;
  samples.reserve(iters);

  using Clock = bench::SerializedRdtscClock<true>;

  for(const auto& [c0, c1] : pairs){
    pin_thread(c0);
    auto t0 = Clock::start();
    pin_thread(c1);
    auto t1 = Clock::stop();

    uint64_t diff = (t1.value >= t0.value) ? (t1.value - t0.value) : 0;
    samples.push_back(diff);
  }

  std::ranges::sort(samples);

  return Stats{
    .min = samples.front(),
    .p50 = samples[iters >> 1],
    .p99 = samples[static_cast<std::size_t>(iters * 0.99)],
    .max = samples.back()
  };
}

static void print_stats(std::string_view name, const Stats& s, std::string_view unit){
#if defined(__cpp_lib_print)
  std::println("{:<32} | min: {:>4} | p50: {:>4} | p99: {:>5} | max: {:>6} {}", name, s.min, s.p50, s.p99, s.max, unit);
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
    temp += i;
    bench::do_not_optimize(temp);
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

#if defined(__cpp_lib_print)
  std::println("thread migration latency across {} cores", cores_available);
#else
  std::cout << "thread migration latency across " << cores_available << " cores \n";
#endif

  if(cores_available > 1){
    auto stats = measure_migration_latency(cores_available);
    print_stats("Pseduorandom core hop", stats, bench::SerializedRdtscClock<true>::unit);
  }else{
#if defined(__cpp_lib_print)
    std::println("skipped, hardware_concurrency() returned a number <= 1");
#else
    std::cout << "skipped, hardware_concurrency() returned a number <= 1\n";
#endif
  }

return 0;
}
