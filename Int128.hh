#pragma once

#include <concepts>
#include <cstdint>
#include <span>

struct Int128 {

    uint64_t words[2]{0, 0};

    constexpr Int128() = default;
    constexpr Int128(uint64_t low, uint64_t high)
    : words{low, high}
    {}

    template<std::integral T>
    constexpr Int128(T val) noexcept{
        if constexpr(std::is_signed_v<T>){
            const auto v = static_cast<int64_t>(val);
            words[0] = static_cast<uint64_t>(v);
            words[1] = v < 0 ? ~0ULL : 0ULL;
        }else{
            words[0] = static_cast<uint64_t>(val);
            words[1] = 0ULL;
        }
    }

    static constexpr Int128 product(const int64_t a, const int64_t b) {
        const uint64_t u = static_cast<uint64_t>(a);
        const uint64_t v = static_cast<uint64_t>(b);

        const uint64_t u0 = static_cast<uint32_t>(u);
        const uint64_t u1 = u >> 32;

        const uint64_t v0 = static_cast<uint32_t>(v);
        const uint64_t v1 = v >> 32;

        const uint64_t p0 = u0 * v0;
        const uint64_t p1 = u0 * v1;
        const uint64_t p2 = u1 * v0;
        const uint64_t p3 = u1 * v1;

        const uint64_t low_mid = (p0 >> 32)
                + (p1 & 0xFFFFFFFFULL)
                + (p2 & 0xFFFFFFFFULL);

        const uint64_t high_mid = (p1 >> 32)
                + (p2 >> 32)
                + (low_mid >> 32);

        uint64_t low = (low_mid << 32) | (p0 & 0xFFFFFFFFULL);
        uint64_t high = p3 + high_mid;

        if(a < 0){
            high -= v;
        }
        if(b < 0){
            high -= u;
        }

        return Int128{low, high};
    }

    constexpr void add(const Int128& other) {
        const uint64_t new_low = words[0] + other.words[0];
        const bool carry = (new_low < words[0]);
        const uint64_t new_high = words[1] + other.words[1] + (carry ? 1ULL : 0ULL);

        words[0] = new_low;
        words[1] = new_high;
    }

    constexpr void subtract(const Int128& other) {
        const bool borrow = (words[0] < other.words[0]);
        const uint64_t new_low = words[0] - other.words[0];
        const uint64_t new_high = words[1] - other.words[1] - (borrow ? 1ULL : 0ULL);

        words[0] = new_low;
        words[1] = new_high;
    }

    constexpr explicit operator bool() const noexcept{
        return (words[0] | words[1]) != 0;
    }

    constexpr explicit operator int64_t() const noexcept{
        return static_cast<int64_t>(words[0]);
    }

    constexpr explicit operator uint64_t() const noexcept{
        return words[0];
    }

    std::span<const std::byte, 16> asBytes() const {
        return std::as_bytes(std::span<const uint64_t, 2>(words));
    }
};

