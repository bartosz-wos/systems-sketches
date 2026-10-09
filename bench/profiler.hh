#pragma once

#include <cstdio>
#include <cstdint>
#include <chrono>
#include <string_view>
#include <concepts>
#include <type_traits>
#include <x86intrin.h>

#if __has_include(<print>)
#  include <print>
#endif

#if !defined(__cpp_lib_print)
#  include <iostream>
#endif

namespace bench{

template<typename T>
inline void do_not_optimize(T&& value) noexcept{
  if constexpr(std::is_const_v<std::remove_reference_t<T>>){
    asm volatile("" : : "r,m"(value) : "memory");
  }else{
    asm volatile("" : "+r,m"(value) : : "memory");
  }
}

template<typename C>
concept Clock = requires(typename C::time_point t){
  typename C::time_point;
  { C::unit }           -> std::convertible_to<std::string_view>;
  { C::start() }        -> std::same_as<typename C::time_point>;
  { C::stop() }         -> std::same_as<typename C::time_point>;
  { C::elapsed(t, t) }  -> std::convertible_to<uint64_t>;
};

struct ChronoClock{
  using time_point = std::chrono::steady_clock::time_point;
  static constexpr std::string_view unit = "ns";

  static time_point start() noexcept{
    return std::chrono::steady_clock::now();
  }

  static time_point stop() noexcept{
    return std::chrono::steady_clock::now();
  }

  static uint64_t elapsed(time_point start, time_point end) noexcept{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
    );
  }
};

struct EmptyCoreTracker{};

struct CoreTracker{
  uint32_t cpu_id{ 0 };

  [[nodiscard]]
  constexpr uint32_t cpu() const noexcept{
    return cpu_id & 0xFFF;
  }

  [[nodiscard]]
  constexpr uint32_t numa_node() const noexcept{
    return cpu_id >> 12;
  }
};

template<typename Value, typename Metadata = EmptyCoreTracker>
struct Point{
  Value value{};
  [[no_unique_address]] Metadata meta{};
};

struct RdtscClock{
  using time_point = uint64_t;
  static constexpr std::string_view unit = "cycles";

  static time_point start() noexcept{
    return __rdtsc();
  }

  static time_point stop() noexcept{
    return __rdtsc();
  }

  static uint64_t elapsed(time_point start, time_point end) noexcept{
    return end - start;
  }
};

template<bool TrackCore = false>
struct SerializedRdtscClock{
  using metadata_type = std::conditional_t<TrackCore, CoreTracker, EmptyCoreTracker>;
  using time_point    = Point<uint64_t, metadata_type>;
  static constexpr std::string_view unit = "cycles";

  static time_point start() noexcept{
    _mm_lfence();
    if constexpr(TrackCore){
      unsigned int temp;
      uint64_t tsc = __rdtscp(&temp);
      _mm_lfence();
      return { tsc, CoreTracker{temp} };
    }else{
      uint64_t tsc = __rdtsc();
      return { tsc, {} };
    }
  }

  static time_point stop() noexcept{
    unsigned int temp;
    uint64_t tsc = __rdtscp(&temp);
    _mm_lfence();

    if constexpr(TrackCore){
      return { tsc, CoreTracker{temp} };
    }else{
      return { tsc, {} };
    }
  }

  static uint64_t elapsed(time_point start, time_point end) noexcept{
    if constexpr(TrackCore){
      if(start.meta.cpu_id != end.meta.cpu_id){
#if defined(__cpp_lib_print)
        std::println(stderr, "Core migrated: {} (node {}) to {} (node {})",
          start.meta.cpu(), start.meta.numa_node(),
          end.meta.cpu(), end.meta.numa_node()
        );
#else
        std::cerr << "Core migration detected\n";
#endif
      }
    }

    return end.value - start.value;
  }
};

struct RdtscpClock{
  using time_point = uint64_t;
  static constexpr std::string_view unit = "cycles";

  static time_point start() noexcept{
    unsigned int temp;
    return __rdtscp(&temp);
  }

  static time_point stop() noexcept{
    unsigned int temp;
    return __rdtscp(&temp);
  }

  static uint64_t elapsed(time_point start, time_point end) noexcept{
    return end - start;
  }
};

template<Clock C = RdtscClock>
struct Timer{
  std::string_view label;
  typename C::time_point start;

  explicit Timer(std::string_view label = "Scope") noexcept
    : label{ label }
    , start{ C::start() }
    {}

  ~Timer(){
    auto end = C::stop();
    auto diff = C::elapsed(start, end);

    #if defined(__cpp_lib_print)
      std::println("[{}] Elapsed: {} {}", label, diff, C::unit);
    #else
      std::cout << "[" << label << "] Elapsed: " << diff << " " << C::unit << '\n';
    #endif
  }

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

};

using CycleTimer                      = Timer<RdtscClock>;
using ChronoTimer                     = Timer<ChronoClock>;
using SerializedCycleTimer            = Timer<SerializedRdtscClock<>>;
using CoreTrackedSerializedCycleTimer = Timer<SerializedRdtscClock<true>>;

} // namespace bench
