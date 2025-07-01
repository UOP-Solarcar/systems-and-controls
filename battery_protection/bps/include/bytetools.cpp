#include "bytetools.hpp"

namespace bytetools {
template <typename T, typename U> U unsafe_cast(T *t) { return *(U *)t; }

template <typename T> T identity(T t) noexcept { return t; }

template <typename T, const size_t N> struct Array {
  T bytes[N];
};

template <typename T>
constexpr T
from_ne_bytes(Array<uint8_t, sizeof(T) / sizeof(uint8_t)> value) noexcept {
  return *(T *)&value;
}

template <typename T>
constexpr Array<uint8_t, sizeof(T) / sizeof(uint8_t)>
to_ne_bytes(T value) noexcept {
  return *(Array<uint8_t, sizeof(T) / sizeof(uint8_t)> *)&value;
}

template <const size_t N>
Array<uint8_t, N> bswap(Array<uint8_t, N> value) noexcept {
  Array<uint8_t, N> out{};
  for (size_t i = 0; i < N; i++) {
    out.bytes[N - 1 - i] = value.bytes[i];
  }
  return out;
}

constexpr Array<uint8_t, sizeof(uint16_t) / sizeof(uint8_t)>
bswap(Array<uint8_t, sizeof(uint16_t) / sizeof(uint8_t)> value) noexcept {
  return to_ne_bytes(__builtin_bswap16(from_ne_bytes<uint16_t>(value)));
}

constexpr Array<uint8_t, sizeof(uint32_t) / sizeof(uint8_t)>
bswap(Array<uint8_t, sizeof(uint32_t) / sizeof(uint8_t)> value) noexcept {
  return to_ne_bytes(__builtin_bswap32(from_ne_bytes<uint32_t>(value)));
}

constexpr Array<uint8_t, sizeof(uint64_t) / sizeof(uint8_t)>
bswap(Array<uint8_t, sizeof(uint64_t) / sizeof(uint8_t)> value) noexcept {
  return to_ne_bytes(__builtin_bswap64(from_ne_bytes<uint64_t>(value)));
}

constexpr uint16_t int_bswap(uint16_t value) noexcept {
  return from_ne_bytes<uint16_t>(bswap(to_ne_bytes(value)));
}

constexpr int16_t int_bswap(int16_t value) noexcept {
  return from_ne_bytes<int16_t>(bswap(to_ne_bytes(value)));
}

constexpr uint32_t int_bswap(uint32_t value) noexcept {
  return from_ne_bytes<uint32_t>(bswap(to_ne_bytes(value)));
}

constexpr int32_t int_bswap(int32_t value) noexcept {
  return from_ne_bytes<int32_t>(bswap(to_ne_bytes(value)));
}

constexpr uint64_t int_bswap(uint64_t value) noexcept {
  return from_ne_bytes<uint64_t>(bswap(to_ne_bytes(value)));
}

constexpr int64_t int_bswap(int64_t value) noexcept {
  return from_ne_bytes<int64_t>(bswap(to_ne_bytes(value)));
}

template <typename T> T dangerous_bswap(T t) {
  return from_ne_bytes<T>(bswap(to_ne_bytes(t)));
}

template <typename T>
constexpr Array<uint8_t, sizeof(T) / sizeof(uint8_t)>
to_le_bytes(T value) noexcept {
  if (to_ne_bytes<uint32_t>(2).bytes[0] == 0) {
    return bswap(to_ne_bytes(value));
  }
  return to_ne_bytes(value);
}

template <typename T>
constexpr Array<uint8_t, sizeof(T) / sizeof(uint8_t)>
to_be_bytes(T value) noexcept {
  if (to_ne_bytes<uint32_t>(2).bytes[0] == 2) {
    return bswap(to_ne_bytes(value));
  }
  return to_ne_bytes(value);
}
} // namespace bytetools
