// Copyright 2020 Håkan Lundvall
// Licence: MIT


#include "bitpack.h"

// The template arguments of bitpack::field decide how the value given in field::set or retrieved in 
// field::get is placed within the buffer.
// The template arguments are, from right to left:
// * how many bits to place
// * start bit position within the buffer
// repeat any number of times for fields that are non-contiguous  

std::array<uint8_t, 4> global_buffer{};
void set_32bit_LE(uint32_t v)
{
    bitpack::field<24,8,16,8,8,8,0,8>::set(begin(global_buffer), v);
}

void set_16bit_BE(uint32_t v)
{
    bitpack::field<0,16>::set(begin(global_buffer), v);
}

int main()
{
    std::array<uint8_t, 4> buffer{};

    // set 8 bit value at pos 8 value in buffer: 00000000111111110000000000000000
    //                                                   ^^^^^^^^
    bitpack::field<8,8>::set(begin(buffer), 255);

    // get 8 bit value at pos 4 value in buffer: 00000000111111110000000000000000
    //                                               ^^^^^^^^
    return bitpack::field<4,8>::get(cbegin(buffer));
}
