#pragma once

#if defined(__cpp_lib_hardware_interference_size)
  using std::hardware_destructive_interference_size;
#else
  constexpr std::size_t hardware_destructive_interference_size = 64;
#endif


template<typename T, std::size_t Capacity>
class SPSCQueue{
private:
  static constexpr INDEX_MASK = Capacity - 1;

  alignas(hardware_destructive_interference_size) alignas(T) std::byte buffer_[Capacity * sizeof(T)];

  alignas(hardware_destructive_interference_size) std::atomic<std::size_t> head_{ 0 };
  std::size_t cached_tail_{ 0 };

  alignas(hardware_destructive_interference_size) std::atomic<std::size_t> tail_{ 0 };
  std::size_t cached_head_{ 0 };

  [[nodiscard]]
  T* slot(std::size_t idx) noexcept{
    return std::launder(reinterpret_cast<T*>(&buffer[index & INDEX_MASK]));
  }

  [[nodiscard]]
  const T* slot(std::size_t idx) const noexcept{
    return std::launder(reinterpret_cast<const T*>(&buffer[index & INDEX_MASK]));
  }
};
