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

struct field_specifier {
  constexpr field_specifier() = default;
  constexpr field_specifier(const uint32_t pos, const uint32_t bits)
      : pos{pos}, bits{bits} {}
  constexpr field_specifier(const field_specifier &) = default;
  uint32_t pos = 0;
  uint32_t bits = 0;
};

/**
 * @class field_object
 * @brief Represents a collection of field specifiers used for packing and
 * unpacking data.
 *
 * The field_object class provides methods for setting and getting values based
 * on the specified field specifiers. It supports both variadic and
 * container-based constructors for specifying the field specifiers. The field
 * specifiers define the position and number of bits for each field in the
 * packed data.
 */
class field_object {
public:
  template <typename... Args>
  /**
   * @brief Constructs a `field_object` with the specified position and number
   * of bits.
   *
   * @param pos The position of the first field.
   * @param bits The number of bits in the first field.
   * @param args Additional fields.
   */
  constexpr field_object(uint32_t pos, uint32_t bits, Args... args)
      : fields_{copy(field_specifiers(pos, bits, args...))},
        count_{sizeof...(args) / 2 + 1} {}

  template <typename Container>
  requires std::ranges::sized_range<Container> &&
      std::same_as<typename Container::value_type, field_specifier>
  /**
   * @brief Constructs a field_object with the given fields.
   *
   * @param fields The container of fields.
   */
  constexpr field_object(const Container &fields)
      : fields_{copy(fields)}, count_{fields.size()} {}

  /**
   * @brief Packs a value into the buffer according to field specifiers given in
   * the constructor.
   *
   * @tparam InputIt The type of the iterator.
   * @param first The iterator pointing to the beginning of the buffer.
   * @param v The value to pack into the buffer.
   */
  template <class InputIt> constexpr void set(InputIt first, uint32_t v) const {
    uint32_t result_shift = 0;
    for (std::size_t i = count_; i > 0; --i) {
      pack(fields_[i - 1], first, v >> result_shift);
      result_shift += fields_[i - 1].bits;
    }
  }

  /**
   * @brief Retrieves a value from the given input iterator.
   *
   * @tparam InputIt The type of the input iterator.
   * @param first The input iterator pointing to the buffer containing the value
   * to retrieve.
   * @return The retrieved value.
   */
  template <class InputIt> constexpr uint32_t get(const InputIt first) const {
    uint32_t v{0};
    uint32_t result_shift = 0;
    for (auto i = count_; i > 0; --i) {
      v |= unpack(fields_[i - 1], first, result_shift);
      result_shift += fields_[i - 1].bits;
    }
    return v;
  }

private:
  template <typename... Args>
  constexpr auto field_specifiers(uint32_t pos, uint32_t bits, Args... args) {
    std::array<field_specifier, 1 + sizeof...(args)> result{};
    auto rest = field_specifiers(args...);
    result[0] = field_specifier{pos, bits};
    std::copy(std::cbegin(rest), std::cend(rest), std::begin(result) + 1);
    return result;
  }

  constexpr auto field_specifiers(uint32_t pos, uint32_t bits) {
    return std::array<field_specifier, 1>{{{pos, bits}}};
  }

  template <class InputIt>
  constexpr void pack(const field_specifier &fs, InputIt start,
                      uint32_t v) const {
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

  template <class InputIt>
  constexpr uint32_t unpack(const field_specifier fs, InputIt start,
                            uint32_t result_shift) const {
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

  template <typename Container, std::size_t... I>
  static constexpr auto copy_impl(const Container &fields,
                                  std::index_sequence<I...>) {
    return std::array<const field_specifier, 32>{{fields[I]...}};
  }

  template <typename Container>
  static constexpr auto copy(const Container &fields) {
    std::array<field_specifier, 32> tmp;
    std::copy(std::cbegin(fields), std::cend(fields), std::begin(tmp));
    return copy_impl(tmp, std::make_index_sequence<32>{});
  }

  const std::array<const field_specifier, 32> fields_;
  const std::size_t count_;
};

} // namespace bitpack
