// Copyright 2020 Håkan Lundvall
// Licence: MIT

#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <functional>
#include <numeric>
#include <ranges>
#include <type_traits>

namespace bitpack {

constexpr uint32_t shift_left_and_add(const uint32_t a, const uint32_t b) {
  return (a << 8) + b;
}

template <int pos, int bits, int... T> struct field {
  static constexpr uint32_t start_index = pos / 8;
  static constexpr uint32_t bit_offset = pos % 8;
  static constexpr uint32_t byte_count = 1 + (bit_offset + bits - 1) / 8;
  static constexpr uint32_t shift = byte_count * 8 - bit_offset - bits;
  static constexpr uint32_t mask = uint32_t((uint64_t(1) << bits) - 1);
  static constexpr uint32_t shifted_mask = (mask << shift);

  // Set the value given in v in the buffer pointed to by the iterator first
  // according to the bit pattern given in template arguments pos, bits, T...
  template <class InputIt>
  static constexpr void set(InputIt first, uint32_t v) {
    set_with_size(first, v);
  }

  // Get the value from the buffer pointed to by the iterator first
  // according to the bit pattern given in template arguments pos, bits, T...
  template <class InputIt> static constexpr uint32_t get(InputIt start) {
    return get_with_size(start).first;
  }

  template <std::size_t S = 0, class InputIt>
  static constexpr auto get_with_size(InputIt start) ->
      typename std::enable_if_t<S == sizeof...(T),
                                std::pair<uint32_t, uint32_t>> {
    return {do_get(start), bits};
  }

  template <std::size_t S = 0, class InputIt>
  static constexpr auto get_with_size(InputIt start) ->
      typename std::enable_if_t<S != sizeof...(T),
                                std::pair<uint32_t, uint32_t>> {
    auto res = field<T...>::get_with_size(start);
    return {res.first + (do_get(start) << res.second), res.second + bits};
  }

  template <std::size_t S = 0, class InputIt>
  static constexpr auto set_with_size(InputIt start, uint32_t v) ->
      typename std::enable_if_t<S == sizeof...(T), uint32_t> {
    do_set(start, v);
    return bits;
  }

  template <std::size_t S = 0, class InputIt>
  static constexpr auto set_with_size(InputIt start, uint32_t v) ->
      typename std::enable_if_t<S != sizeof...(T), uint32_t> {
    auto res = field<T...>::set_with_size(start, v);
    do_set(start, v >> res);
    return res + bits;
  }

private:
  template <class InputIt> static constexpr uint32_t do_get(InputIt start) {
    const auto start_pos = start + start_index;
    const auto end_pos = start_pos + byte_count;
    const auto part =
        std::accumulate(start_pos, end_pos, 0, shift_left_and_add);
    return (part >> shift) & mask;
  }

  template <class InputIt>
  static constexpr void do_set(InputIt start, uint32_t v) {
    const auto start_pos = start + start_index;
    const auto end_pos = start_pos + byte_count;

    uint8_t c[4]{0};

    const uint32_t vc32{(v << shift) & shifted_mask};
    const uint8_t vc[4]{uint8_t((vc32)&0xff), uint8_t((vc32 >> 8) & 0xff),
                        uint8_t((vc32 >> 16) & 0xff),
                        uint8_t((vc32 >> 24) & 0xff)};

    const uint32_t m32 = ~shifted_mask;
    const uint8_t m[4]{uint8_t((m32)&0xff), uint8_t((m32 >> 8) & 0xff),
                       uint8_t((m32 >> 16) & 0xff),
                       uint8_t((m32 >> 24) & 0xff)};

    std::transform(start_pos, end_pos, std::crbegin(m) + (4 - byte_count),
                   std::begin(c), std::bit_and<uint8_t>());
    std::transform(std::cbegin(c), std::cbegin(c) + byte_count,
                   std::rbegin(vc) + (4 - byte_count), start_pos,
                   std::bit_or<uint8_t>());
  }
};

struct field_specifier {
  constexpr field_specifier() = default;
  constexpr field_specifier(const uint32_t pos, const uint32_t bits)
      : pos{pos}, bits{bits} {}
  uint32_t pos = 0;
  uint32_t bits = 0;
};

class field_object {
public:
  template <typename Container>
  requires std::ranges::sized_range<Container> &&
      std::same_as<typename Container::value_type, field_specifier>
  constexpr field_object(const Container &container)
      : container_{copy(container)}, count_{container.size()} {}

  template <class InputIt> constexpr void set(InputIt first, uint32_t v) const {
    pack(container_, count_, first, v);
  }

  template <class InputIt> constexpr uint32_t get(const InputIt first) const {
    return unpack(container_, count_, first);
  }

private:
  template <typename Container, class InputIt>
  requires std::ranges::sized_range<Container> &&
      std::same_as<typename Container::value_type, field_specifier>
  static constexpr void pack(const Container &fs, std::size_t count,
                             InputIt start, uint32_t v) {
    uint32_t result_shift = 0;
    for (std::size_t i = std::min(count, sizeof(fs) / sizeof(field_specifier));
         i > 0; --i) {
      pack(fs[i - 1], start, v >> result_shift);
      result_shift += fs[i - 1].bits;
    }
  }

  template <class InputIt>
  static constexpr void pack(const field_specifier fs, InputIt start,
                             uint32_t v) {
    const auto pos = fs.pos;
    const auto bits = fs.bits;
    const uint32_t start_index = pos / 8;
    const uint32_t bit_offset = pos % 8;
    const uint32_t byte_count = 1 + (bit_offset + bits - 1) / 8;
    const uint32_t shift = byte_count * 8 - bit_offset - bits;
    const uint32_t mask = uint32_t((uint64_t(1) << bits) - 1);
    const uint32_t shifted_mask = (mask << shift);

    const auto start_pos = start + start_index;
    const auto end_pos = start_pos + byte_count;

    uint8_t c[4]{0};

    const uint32_t vc32{(v << shift) & shifted_mask};
    const uint8_t vc[4]{uint8_t((vc32)&0xff), uint8_t((vc32 >> 8) & 0xff),
                        uint8_t((vc32 >> 16) & 0xff),
                        uint8_t((vc32 >> 24) & 0xff)};

    const uint32_t m32 = ~shifted_mask;
    const uint8_t m[4]{uint8_t((m32)&0xff), uint8_t((m32 >> 8) & 0xff),
                       uint8_t((m32 >> 16) & 0xff),
                       uint8_t((m32 >> 24) & 0xff)};

    std::transform(start_pos, end_pos, std::crbegin(m) + (4 - byte_count),
                   std::begin(c), std::bit_and<uint8_t>());
    std::transform(std::cbegin(c), std::cbegin(c) + byte_count,
                   std::rbegin(vc) + (4 - byte_count), start_pos,
                   std::bit_or<uint8_t>());
  }

  template <typename Container, class InputIt>
  requires std::ranges::sized_range<Container> &&
      std::same_as<typename Container::value_type, field_specifier>
  static constexpr uint32_t unpack(const Container &fs, std::size_t count,
                                   InputIt start) {
    uint32_t v{0};
    uint32_t result_shift = 0;
    for (auto i = std::min(count, sizeof(fs) / sizeof(field_specifier)); i > 0;
         --i) {
      v |= unpack(fs[i - 1], start, result_shift);
      result_shift += fs[i - 1].bits;
    }
    return v;
  }

  template <class InputIt>
  static constexpr uint32_t unpack(const field_specifier fs, InputIt start,
                                   uint32_t result_shift) {
    const auto pos = fs.pos;
    const auto bits = fs.bits;
    const uint32_t start_index = pos / 8;
    const uint32_t bit_offset = pos % 8;
    const uint32_t byte_count = 1 + (bit_offset + bits - 1) / 8;
    const uint32_t shift = byte_count * 8 - bit_offset - bits;
    const uint32_t mask = uint32_t((uint64_t(1) << bits) - 1);
    const auto start_pos = start + start_index;
    const auto end_pos = start_pos + byte_count;
    const auto part =
        std::accumulate(start_pos, end_pos, 0, shift_left_and_add);
    return ((part >> shift) & mask) << result_shift;
  }

  template <typename Container>
  constexpr std::array<field_specifier, 32> copy(const Container &container) {
    std::array<field_specifier, 32> res;
    std::copy(std::cbegin(container), std::cend(container), std::begin(res));
    return res;
  }

  const std::array<field_specifier, 32> container_;
  std::size_t count_;
};

} // namespace bitpack
