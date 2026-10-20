#pragma once

#include <cstddef>
#include <utility>

struct alignas(std::max_align_t) Header {
    std::size_t size;
    std::size_t free;
};

static constexpr std::size_t ALIGNMENT = alignof(std::max_align_t);
static constexpr std::size_t HEADER_SIZE = sizeof(Header);

class FirstFitAllocator {
private:
    std::byte* buffer_{ nullptr };
    std::size_t total_size_{ 0 };
    std::size_t total_free_{ 0 };
    int free_block_count_{ 0 };

    [[nodiscard]]
    static constexpr std::size_t align_up(std::size_t n, std::size_t alignment) noexcept{
      return (n + alignment - 1) & ~(alignment - 1);
    }

    [[nodiscard]]
    static constexpr std::uintptr_t align_up(std::uintptr_t n, std::size_t alignment) noexcept{
      return (n + alignment - 1) & ~static_cast<std::uintptr_t>(alignment - 1);
    }

    [[nodiscard]]
    static Header* to_header(std::byte* ptr) noexcept{
        return reinterpret_cast<Header*>(ptr);
    }

    [[nodiscard]]
    static const Header* to_header(const std::byte* ptr) noexcept{
        return reinterpret_cast<const Header*>(ptr);
    }

    [[nodiscard]]
    static std::byte* to_bytes(Header* header) noexcept{
        return reinterpret_cast<std::byte*>(header);
    }

    [[nodiscard]]
    static const std::byte* to_bytes(const Header* header) noexcept{
        return reinterpret_cast<const std::byte*>(header);
    }

    [[nodiscard]]
    static void* payload(Header* header) noexcept{
        return to_bytes(header) + HEADER_SIZE;
    }

    [[nodiscard]]
    static Header* from_payload(void* p) noexcept{
        return to_header(static_cast<std::byte*>(p) - HEADER_SIZE);
    }

    [[nodiscard]]
    static Header* next_block(Header* header) noexcept{
        return to_header(to_bytes(header) + HEADER_SIZE + header->size);
    }

    [[nodiscard]]
    static const Header* next_block(const Header* header) noexcept{
        return to_header(to_bytes(header) + HEADER_SIZE + header->size);
    }

public:
    explicit FirstFitAllocator(std::byte* buffer, std::size_t size) noexcept
    : buffer_{ buffer }
    , total_size_{ size }
    , total_free_{ size < HEADER_SIZE ? 0 : size - HEADER_SIZE }
    , free_block_count_{ 1 }
    {
      if(!buffer || size < HEADER_SIZE) [[unlikely]]{
        buffer_ = nullptr;
        total_size_ = 0;
        total_free_ = 0;
        free_block_count_ = 0;
        return;
      }

      Header* block = to_header(buffer_);
      block->size = total_free_;
      block->free = 1;
    }

    explicit FirstFitAllocator(char* buffer, std::size_t size) noexcept
    : FirstFitAllocator(reinterpret_cast<std::byte*>(buffer), size)
    {}

    ~FirstFitAllocator() = default;

    FirstFitAllocator(const FirstFitAllocator&) = delete;
    FirstFitAllocator& operator=(const FirstFitAllocator&) = delete;

    FirstFitAllocator(FirstFitAllocator&& other) noexcept
    : buffer_{ std::exchange(other.buffer_, nullptr) }
    , total_size_{ std::exchange(other.total_size_, 0) }
    , total_free_{ std::exchange(other.total_free_, 0) }
    , free_block_count_{ std::exchange(other.free_block_count_, 0) }
    {}

    FirstFitAllocator& operator=(FirstFitAllocator&& other) noexcept{
      if(this != &other){
        buffer_ = std::exchange(other.buffer_, nullptr);
        total_size_ = std::exchange(other.total_size_, 0);
        total_free_ = std::exchange(other.total_free_, 0);
        free_block_count_ = std::exchange(other.free_block_count_, 0);
      }
      return *this;
    }

    [[nodiscard]]
    void* allocate(std::size_t n) noexcept{
      std::byte* ptr = buffer_;
      const std::byte* end = buffer_ + total_size_;

      while(ptr < end){
        Header* cur = to_header(ptr);

        if(cur->free && cur->size >= n){
          const std::size_t block_size = cur->size;

          if(block_size - n > HEADER_SIZE){
            cur->size = n;
            cur->free = 0;

            Header* new_block = next_block(cur);
            new_block->size = (block_size - n) - HEADER_SIZE;
            new_block->free = 1;

            total_free_ -= (n + HEADER_SIZE);
          }else{
            cur->free = 0;
            --free_block_count_;
            total_free_ -= block_size;
          }

          return payload(cur);
        }

        ptr = to_bytes(next_block(cur));
      }

      return nullptr;
    }

    void deallocate(void* p) noexcept{
      if(!p){
        return;
      }

      Header* header = from_payload(p);
      Header* prev = nullptr;
      std::byte* ptr = buffer_;
      const std::byte* end = buffer_ + total_size_;

      while(ptr < end){
        Header* cur = to_header(ptr);
        if(cur == header){
          break;
        }
        prev = cur;
        ptr = to_bytes(next_block(cur));
      }

      header->free = 1;
      ++free_block_count_;
      total_free_ += header->size;

      std::byte* next = to_bytes(next_block(header));
      if(next < end){
        Header* next_header = to_header(next);
        if(next_header->free){
          header->size += HEADER_SIZE + next_header->size;
          --free_block_count_;
          total_free_ += HEADER_SIZE;
        }
      }

      if(prev != nullptr && prev->free){
        prev->size += HEADER_SIZE + header->size;
        --free_block_count_;
        total_free_ += HEADER_SIZE;
      }
    }

    [[nodiscard]]
    std::size_t largest_free_block() const noexcept{
      std::size_t mx_block = 0;
      const std::byte* ptr = buffer_;
      const std::byte* end = buffer_ + total_size_;

      while(ptr < end){
        const Header* cur = to_header(ptr);
        if(cur->free && cur->size > mx_block){
          mx_block = cur->size;
        }
        ptr = to_bytes(next_block(cur));
      }

      return mx_block;
    }

    [[nodiscard]]
    int free_block_count() const noexcept{
        return free_block_count_;
    }

    [[nodiscard]]
    std::size_t total_free() const noexcept{
        return total_free_;
    }
};
