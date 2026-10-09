#pragma once

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

struct Timer{
  using Clock = std::chrono::steady_clock;

  std::string_view label;
  Clock::time_point start;

  explicit Timer(std::string_view label = "Scope") noexcept
    : label{ label }
    , start{ Clock::now() }
    {}

  ~Timer(){
    auto end = Clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    #if defined(__cpp_lib_print)
      std::println("[{}] Elapsed: {} ns", label, ns);
    #else
      std::cout << "[" << label << "] Elapsed: " << ns << " ns\n";
    #endif
  }

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

};

} // namespace bench
