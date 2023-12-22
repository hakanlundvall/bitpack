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
constexpr bitpack::field_object f{0, 4, 8, 4, 4, 4, 16, 4, 12, 4, 20, 4, 24, 8};
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
  std::uniform_int_distribution<uint32_t> dis(1, 16);
  std::uniform_int_distribution<uint32_t> val_dis(0, UINT32_MAX);

  for (int i = 0; i < 1000; ++i) {
    std::array<uint8_t, 8> buffer{};
    std::vector<bitpack::field_specifier> fields1;
    std::vector<bitpack::field_specifier> fields2;

    uint32_t total_bits = 0;
    uint32_t total_bits1 = 0;
    uint32_t total_bits2 = 0;

    while (total_bits < 64) {
      auto field_size = dis(gen);
      if (total_bits1 + field_size > 32) {
        field_size = 32 - total_bits1;
      }
      if (field_size > 0)
        fields1.push_back(bitpack::field_specifier{total_bits, field_size});
      total_bits1 += field_size;
      total_bits += field_size;

      field_size = dis(gen);
      if (total_bits2 + field_size > 32) {
        field_size = 32 - total_bits2;
      }
      if (field_size > 0)
        fields2.push_back(bitpack::field_specifier{total_bits, field_size});
      total_bits2 += field_size;
      total_bits += field_size;
    }
    std::shuffle(fields1.begin(), fields1.end(), gen);
    std::shuffle(fields2.begin(), fields2.end(), gen);

    bitpack::field_object f1{fields1};
    bitpack::field_object f2{fields2};

    uint32_t v1 = val_dis(gen);
    uint32_t v2 = val_dis(gen);

    f1.set(begin(buffer), v1);
    auto b1 = f1.get(cbegin(buffer));
    auto b2 = f2.get(cbegin(buffer));
    ASSERT_EQ(b1, v1);
    ASSERT_EQ(b2, 0);

    f2.set(begin(buffer), v2);
    b1 = f1.get(cbegin(buffer));
    b2 = f2.get(cbegin(buffer));
    ASSERT_EQ(b1, v1);
    ASSERT_EQ(b2, v2);
  }
}
