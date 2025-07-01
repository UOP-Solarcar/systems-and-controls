#ifndef BYTETOOLS_H
#define BYTETOOLS_H

#include <stddef.h>
#include <stdint.h>

namespace bytetools {
template <typename T, typename U> U unsafe_cast(T *t);

template <typename T> T identity(T t) noexcept;

template <typename T, const size_t N> struct Array;

template <typename T>
constexpr T
from_ne_bytes(Array<uint8_t, sizeof(T) / sizeof(uint8_t)> value) noexcept;

template <typename T>
constexpr Array<uint8_t, sizeof(T) / sizeof(uint8_t)>
to_ne_bytes(T value) noexcept;

template <const size_t N>
Array<uint8_t, N> bswap(Array<uint8_t, N> value) noexcept;

constexpr Array<uint8_t, sizeof(uint16_t) / sizeof(uint8_t)>
bswap(Array<uint8_t, sizeof(uint16_t) / sizeof(uint8_t)> value) noexcept;

constexpr Array<uint8_t, sizeof(uint32_t) / sizeof(uint8_t)>
bswap(Array<uint8_t, sizeof(uint32_t) / sizeof(uint8_t)> value) noexcept;

constexpr Array<uint8_t, sizeof(uint64_t) / sizeof(uint8_t)>
bswap(Array<uint8_t, sizeof(uint64_t) / sizeof(uint8_t)> value) noexcept;

constexpr uint16_t int_bswap(uint16_t value) noexcept;

constexpr int16_t int_bswap(int16_t value) noexcept;

constexpr uint32_t int_bswap(uint32_t value) noexcept;

constexpr int32_t int_bswap(int32_t value) noexcept;

constexpr uint64_t int_bswap(uint64_t value) noexcept;

constexpr int64_t int_bswap(int64_t value) noexcept;

template <typename T> T dangerous_bswap(T t);

template <typename T>
constexpr Array<uint8_t, sizeof(T) / sizeof(uint8_t)>
to_le_bytes(T value) noexcept;

template <typename T>
constexpr Array<uint8_t, sizeof(T) / sizeof(uint8_t)>
to_be_bytes(T value) noexcept;
} // namespace bytetools

#endif // BYTETOOLS_H
