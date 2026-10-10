#pragma once

namespace parser::wire{

template<typename T>
requires (std::is_trivially_copyable_v<T>)
[[nodiscard]]
inline T read_big_endian(const std::byte* src) noexcept{
  T val;
  std::memcpy(&val, src, sizeof(T));

  if constexpr(std::endian::native == std::endian::little){
    if constexpr(sizeof(T) == 1){
      return val;
    }else if constexpr(sizeof(T) == 2){
      return static_cast<T>(__builtin_bswap16(static_cast<uint16_t>(val)));
    }else if constexpr(sizeof(T) == 4){
      return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(val)));
    }else if constexpr(sizeof(T) == 8){
      return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(val)));
    }
  }

  return val;
}

template<typename T>
requires (std::is_trivially_copyable_v<T>)
inline void write_big_endian(std::byte* dst, T val) noexcept{
  if constexpr(std::endian::native == std::endian::little){
    if constexpr(sizeof(T) == 2){
      val = static_cast<T>(__builtin_bswap16(static_cast<uint16_t>(val)));
    }else if constexpr(sizeof(T) == 4){
      val = static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(val)));
    }else if constexpr(sizeof(T) == 8){
      val = static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(val)));
    }
  }

  std::memcpy(dst, &val, sizeof(T));
}

}
