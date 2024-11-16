#pragma once

#include <cstdint>


typedef std::uint8_t  U8;
typedef std::uint16_t U16;
typedef std::uint32_t U32;
typedef std::uint64_t U64;
typedef std::size_t   size_t;

constexpr size_t log2ceil(U64 x) {
  return x == 1 ? 0 : 1 + log2ceil((x + 1) / 2);
}