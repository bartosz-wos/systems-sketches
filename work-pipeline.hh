#pragma once


#include "simple-bounded-spsc-queue.hh"

namespace pipe{

namespace line{

template<typename Current, typename... Fs>
struct pipeline_types_helper;

// base case, last function
template<typename Current>
struct pipeline_types_helper<Current>{
  using type = std::tuple<Current>;
};

// recursive step. we call F on Current, deduce next and recurse on Later
template<typename Current, typename F, typename... Later>
struct pipeline_types_helper<Current, F, Later...>{
  static_assert(std::is_invocable_v<F, Current>, "pipeline stage types do not agree!");

  using raw_next_type = std::invoke_result_t<F, Current>;

  static_assert(!std::is_void_v<raw_next_type>, "pipeline stage returns void, cannot pass further!");

  using next_type = std::remove_cvref_t<raw_next_type>;

  using type = decltype(std::tuple_cat(
      std::declval<std::tuple<Current>>(),
      std::declval<typename pipeline_types_helper<next_type, Later...>::type>()
  ));
};



// for mapping tuple of types to tuple of SPSC queues of types
template<std::size_t Cap, typename Tuple>
struct queue_tuple_helper;

template<std::size_t Cap, typename... Ts>
struct queue_tuple_helper<Cap, std::tuple<Ts...>>{
  using type = std::tuple<SPSCQueue<Ts, Cap>...>;
};

} //namespace line


template<typename T0, typename... Fs>
using pipeline_types_t = typename line::pipeline_types_helper<std::remove_cvref_t<T0>, Fs...>::type;

template<std::size_t Cap, typename T0, typename... Fs>
using pipeline_queues_t = typename line::queue_tuple_helper<Cap, pipeline_types_t<T0, Fs...>>::type;

template<std::size_t Cap, typename T0, typename... Fs>
class Pipeline{
public:
  static constexpr std::size_t STAGES = sizeof...(Fs);
  static_assert(STAGES > 0, "pipeline must have at least one invocable stage!");

  using all_types = pipeline_types_t<T0, Fs...>;
  using input_type = std::tuple_element_t<0, all_types>;
  using output_type = std::tuple_element_t<STAGES, all_types>;

  explicit Pipeline(Fs... stages);
  ~Pipeline() = default;

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;
  Pipeline(Pipeline&&) = delete;
  Pipeline& operator=(Pipeline&&) = delete;

};

} // namespace pipe
