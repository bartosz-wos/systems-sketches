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
    return std::launder(reinterpret_cast<T*>(&buffer[(index & INDEX_MASK) * sizeof(T)]));
  }

  [[nodiscard]]
  const T* slot(std::size_t idx) const noexcept{
    return std::launder(reinterpret_cast<const T*>(&buffer[(index & INDEX_MASK) * sizeof(T)]));
  }

public:
  constexpr SPSCQueue() noexcept = default;

  ~SPSCQueue(){
    if constexpr(!std::is_trivially_destructible_v<T>){
      std::size_t tail = tail_.load(std::memory_order_relaxed);
      const std::size_t head = head_.load(std::memory_order_relaxed);

      while(tail != head){
        std::destroy_at(slot(tail));
        ++tail;
      }
    }
  }

  SPSCQueue(const SPSCQueue&) = delete;
  SPSCQueue& operator=(const SPSCQueue&) = delete;
  SPSCQueue(SPSCQueue&&) = delete;
  SPSCQueue& operator=(SPSCQueue&&) = delete;
};
