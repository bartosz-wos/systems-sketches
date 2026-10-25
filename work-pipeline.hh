#pragma once


#include "simple-bounded-spsc-queue.hh"

namespace pipe{

namespace line{

// base case, last function
template<typename Current>
struct pipeline_types_helper{
  using type = std::tuple<Current>;
};

// recursive step. we call F on Current, deduce next and recurse on Later
template<typename Current, typename F, typename... Later>
struct pipeline_types_helper<Current, F, Later...>{
  static_assert(std::is_invocable_v<F, Current>, "pipeline stage types do not agree!");

  using raw_next_type = std::invoke_result_t<F, Current>;

  static_assert(!std::is_void_v<raw_next_type>, "pipeline stage returns void, cannot pass further!");

  using next_type = std::remove_cvref_t<raw_next_type>;

  using type = pipeline_types_helper(std::tuple_cat(
      std::declval<std::tuple<Current>>(),
      std::declval<typename pipeline_types_helper<next_type, Later...>::type>()
  ));
}

template<typename T0, typename... Fs>
using pipeline_types_t = typename line::pipeline_types_helper<std::remove_cvref_t<T0>, Fs...>::type;

} //namespace line

} // namespace pipe
