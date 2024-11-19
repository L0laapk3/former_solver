#pragma once

#include "types.h"

#include <array>



constexpr static inline U64 rotl(const U64 x, int k) {
	return (x << k) | (x >> (64 - k));
}

// https://prng.di.unimi.it/xoshiro256plusplus.c
struct Xoshiro256 {
	std::array<U64, 4> s;

	constexpr U64 next(void) {
		const U64 result = rotl(s[0] + s[3], 23) + s[0];

		const U64 t = s[1] << 17;

		s[2] ^= s[0];
		s[3] ^= s[1];
		s[1] ^= s[2];
		s[0] ^= s[3];

		s[2] ^= t;

		s[3] = rotl(s[3], 45);
		return result;
	}
};