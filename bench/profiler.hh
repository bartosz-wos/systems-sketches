#pragma once

#include <cstdint>
#include <chrono>
#include <string_view>
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
  { C::now() }          -> std::same_as<typename C::time_point>;
  { C::elapsed(t, t) }  -> std::convertible_to<uint64_t>;
};

struct ChronoClock{
  using time_point = std::chrono::steady_clock::time_point;
  static constexpr std::string_view unit = "ns";

  static time_point now() noexcept{
    return std::chrono::steady_clock::now();
  }

  static uint64_t elapsed(time_point start, time_point end) noexcept{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
    );
  }
};

struct RdtscClock{
  using time_point = uint64_t;
  static constexpr std::string_view unit = "cycles";

  static time_point now() noexcept{
    return __rdtsc();
  }

  static uint64_t elapsed(time_point start, time_point end) noexcept{
    return end - start;
  }
};

struct RdtscpClock{
  using time_point = uint64_t;
  static constexpr std::string_view unit = "cycles";

  static time_point now() noexcept{
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
    , start{ C::now() }
    {}

  ~Timer(){
    auto end = C::now();
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

using CycleTimer  = Timer<RdtscClock>;
using ChronoTimer = Timer<ChronoClock>;

} // namespace bench
