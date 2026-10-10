#include "wire.hh"

#include <array>
#include <cassert>
#include <iostream>

inline bool test_not_aligned_8_bytes(){
  std::array<std::byte, 16> buffer{};
  constexpr uint64_t val = 0x0123456789ABCDEF;

  parser::wire::write_big_endian(buffer.data() + 1, val);
  return (parser::wire::read_big_endian<uint64_t>(buffer.data() + 1) == val);
}

int main(){
  assert(test_not_aligned_8_bytes());

  std::cout << "tests passed!\n";
  return 0;
}
