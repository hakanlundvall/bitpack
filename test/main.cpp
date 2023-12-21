// Copyright 2020 Håkan Lundvall
// Licence: MIT

#include "bitpack.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <iostream>
#include <random>
#include <sstream>

namespace {


constexpr std::array<uint8_t, 4> buffer{0x12, 0x34, 0x56, 0x78};
constexpr std::array<bitpack::field_specifier, 7> fields{
    bitpack::field_specifier{0, 4},  bitpack::field_specifier{8, 4},
    bitpack::field_specifier{4, 4},  bitpack::field_specifier{16, 4},
    bitpack::field_specifier{12, 4}, bitpack::field_specifier{20, 4},
    bitpack::field_specifier{24, 8}};
constexpr bitpack::field_object f{fields};
static_assert(buffer.size() == 4);
static_assert(f.get(cbegin(buffer)) == 0x13254678);

constexpr auto generate_buffer() {
  std::array<uint8_t, 4> buffer{};
  std::iota(buffer.begin(), buffer.end(), 0);
  f.set(buffer.begin(), 0x13254678);
  return buffer;
}

constexpr std::array<uint8_t, 4> buffer2 = generate_buffer();
static_assert(buffer2 == buffer);

} // namespace

TEST(BitpackTest, RandomValuesAndFields) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dis(
      1, 8); // field sizes from 1 to 8 bits
  std::uniform_int_distribution<uint32_t> val_dis(0,
                                                  UINT32_MAX); // random values

  for (int i = 0; i < 100; ++i) { // run test 100 times
    std::array<uint8_t, 4> buffer{};
    std::vector<bitpack::field_specifier> fields;
    uint32_t total_bits = 0;

    while (total_bits < 32) {
      auto field_size = dis(gen);
      if (total_bits + field_size > 32) {
        field_size = 32 - total_bits;
      }
      fields.push_back(bitpack::field_specifier{total_bits, field_size});
      total_bits += field_size;
    }
    std::shuffle(fields.begin(), fields.end(), gen); // shuffle the fields

    bitpack::field_object f{fields};
    uint32_t v = val_dis(gen); // generate random value
    f.set(begin(buffer), v);
    auto b = f.get(cbegin(buffer));

    std::stringstream ss;
    ss << "Field specifications: " << std::endl;
    for (const auto &field : fields) {
      ss << "Start bit: " << field.pos << ", Field size: " << field.bits
         << std::endl;
    }

    ASSERT_EQ(v, b) << ss.str() << std::endl;
  }
}
