#pragma once

#include <chrono>

#if __has_include(<print>)
#  include <print>
#endif

#if !defined(__cpp_lib_print)
#  include <iostream>
#endif

struct Timer{
  using Clock = std::chrono::steady_clock;

  Clock::time_point start = Clock::now();

  ~Timer(){
    auto end = Clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    #if defined(__cpp_lib_print)
      std::println("Elapsed: {} ns", ns);
    #else
      std::cout << "Elapsed: " << ns << " ns\n";
    #endif
  }
};
