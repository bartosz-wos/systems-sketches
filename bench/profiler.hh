#pragma once

#include <chrono>
#include <string_view>

#if __has_include(<print>)
#  include <print>
#endif

#if !defined(__cpp_lib_print)
#  include <iostream>
#endif

namespace bench{

template<typename T>
inline void do_not_optimize(const T& value) noexcept{
  asm volatile("" : : "r,m"(value) : "memory");
}

template<typename T>
inline void do_not_optimize(T& value) noexcept{
  asm volatile("" : "+r,m"(value) : : "memory");
}

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
