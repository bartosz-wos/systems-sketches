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

struct Timer{
  using Clock = std::chrono::steady_clock;

  Clock::time_point start;
  std::string_view label;

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
