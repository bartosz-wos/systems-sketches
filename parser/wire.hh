#pragma once

namespace parser::wire{

template<typename T>
[[nodiscard]]
inline T read_big_endian(const std::byte* src) noexcept{
  static_assert(std::is_trivially_copyable_v<T>, "T has to be trivially copyable");

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

}
