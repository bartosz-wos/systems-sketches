#pragma once

#include <atomic>
#include <new>
#include <memory>
#include <type_traits>
#include <cstddef>
#include <concepts>
#include <bit>

#if defined(__cpp_lib_hardware_interference_size)
  using std::hardware_destructive_interference_size;
#else
  constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

template<std::size_t N>
concept PowerTwoCapacity = (N >= 2) && std::has_single_bit(N);

template<std::destructible T, std::size_t Capacity>
requires PowerTwoCapacity<Capacity>
class SPSCQueue{
private:
  static constexpr std::size_t INDEX_MASK = Capacity - 1;

  alignas(hardware_destructive_interference_size) alignas(T) std::byte buffer_[Capacity * sizeof(T)];

  alignas(hardware_destructive_interference_size) std::atomic<std::size_t> head_{ 0 };
  std::size_t cached_tail_{ 0 };

  alignas(hardware_destructive_interference_size) std::atomic<std::size_t> tail_{ 0 };
  std::size_t cached_head_{ 0 };

  [[nodiscard]]
  std::byte* raw_slot(std::size_t idx) noexcept{
    return &buffer_[(idx & INDEX_MASK) * sizeof(T)];
  }

  [[nodiscard]] [[maybe_unused]]
  const std::byte* raw_slot(std::size_t idx) const noexcept{
    return &buffer_[(idx & INDEX_MASK) * sizeof(T)];
  }

  [[nodiscard]]
  T* slot(std::size_t idx) noexcept{
    return std::launder(reinterpret_cast<T*>(&buffer_[(idx & INDEX_MASK) * sizeof(T)]));
  }

  [[nodiscard]] [[maybe_unused]]
  const T* slot(std::size_t idx) const noexcept{
    return std::launder(reinterpret_cast<const T*>(&buffer_[(idx & INDEX_MASK) * sizeof(T)]));
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

  template<typename... Args>
  requires std::constructible_from<T, Args...>
  [[nodiscard]]
  bool try_emplace(Args&&... args) &{
    const std::size_t head = head_.load(std::memory_order_relaxed);

    if(head - cached_tail_ == Capacity) [[unlikely]]{
      cached_tail_ = tail_.load(std::memory_order_acquire);
      if(head - cached_tail_ == Capacity){
        return false;
      }
    }

    std::construct_at(reinterpret_cast<T*>(raw_slot(head)), std::forward<Args>(args)...);
    head_.store(head + 1, std::memory_order_release);
    return true;
  }

  [[nodiscard]]
  bool try_push(const T& value) & requires std::copy_constructible<T>{
    return try_emplace(value);
  }

  [[nodiscard]]
  bool try_push(T&& value) & requires std::move_constructible<T>{
    return try_emplace(std::move(value));
  }

  [[nodiscard]]
  bool try_pop(T& value) & noexcept(std::is_nothrow_move_assignable_v<T>) requires std::assignable_from<T&, T&&>{
    const std::size_t tail = tail_.load(std::memory_order_relaxed);

    if(tail == cached_head_){
      cached_head_ = head_.load(std::memory_order_acquire);
      if(tail == cached_head_) [[unlikely]]{
        return false;
      }
    }

    T* const item = slot(tail);
    value = std::move(*item);
    std::destroy_at(item);
    tail_.store(tail + 1, std::memory_order_release);
    return true;
  }

  [[nodiscard]]
  T* front() noexcept{
    const std::size_t tail = tail_.load(std::memory_order_relaxed);

    if(tail == cached_head_) [[unlikely]]{
      cached_head_ = head_.load(std::memory_order_acquire);
      if(tail == cached_head_){
        return nullptr;
      }
    }

    return slot(tail);
  }

  void pop() & noexcept{
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    std::destroy_at(slot(tail));
    tail_.store(tail + 1, std::memory_order_release);
  }

  [[nodiscard]]
  bool empty() const noexcept{
    return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
  }

  [[nodiscard]]
  std::size_t size() const noexcept{
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    const std::size_t head = head_.load(std::memory_order_relaxed);

    const std::size_t diff = head - tail;
    return (diff <= Capacity) ? diff : Capacity;
  }

  [[nodiscard]]
  static constexpr std::size_t capacity() noexcept{
    return Capacity;
  }
};
