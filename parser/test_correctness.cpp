#include "wire.hh"

#include <array>
#include <cassert>
#include <iostream>

inline bool test_not_aligned_8_bytes(){
  using namespace parser;

  std::array<std::byte, 16> buffer{};
  constexpr uint64_t val = 0x0123456789ABCDEF;

  wire::write_big_endian(buffer.data() + 1, val);
  return (wire::read_big_endian<uint64_t>(buffer.data() + 1) == val);
}

inline bool test_header_view(){
  using namespace parser;

  std::array<std::byte, protocol::header_view::SIZE> buffer{};

  wire::write_big_endian<uint16_t>(buffer.data(), 24);
  buffer[2] = static_cast<std::byte>(protocol::MessageType::NewOrder);
  buffer[3] = std::byte{ 0 };

  protocol::header_view view{ buffer.data() };
  bool ret{ true };

  ret |= (view.len() == 24);
  ret |= (view.type() == protocol::MessageType::NewOrder);

  return ret;
}

int main(){
  assert(test_not_aligned_8_bytes());
  assert(test_header_view());

  std::cout << "tests passed!\n";
  return 0;
}
