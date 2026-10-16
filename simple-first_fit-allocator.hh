#pragma once

#include <cstddef>
#include <utility>

struct Header {
    std::size_t size;
    std::size_t free;
};

static constexpr std::size_t HEADER_SIZE = sizeof(Header);

class FirstFitAllocator {
private:
    std::byte* buffer_{ nullptr };
    std::size_t total_size_{ 0 };
    std::size_t total_free_{ 0 };
    int free_block_count_{ 0 };

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
    , total_free_{ size - HEADER_SIZE }
    , free_block_count_{ 1 }
    {
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
        total_size_ = std:exchange(other.total_size_, 0);
        total_free_ = std::exchange(other.total_free_, 0);
        free_block_count_ = std::exchange(other.free_block_count_, 0);
      }
      return *this;
    }

    void* allocate(std::size_t n) {
        return nullptr;
    }

    void deallocate(void* p) {
    }

    std::size_t largest_free_block() const {
    }

    int free_block_count() const {
        return free_block_count_;
    }

    std::size_t total_free() const {
        return total_free_;
    }
};
