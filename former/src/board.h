#pragma once

#include <array>
#include <string_view>
#include <string>
#include <span>
#include <iterator>

#include "types.h"

typedef U64 Score;

struct Board;
struct Board {
	static constexpr size_t WIDTH = 7;
	static constexpr size_t HEIGHT = 9;
	static constexpr size_t SIZE = WIDTH * HEIGHT;

	static constexpr U64 MASK_LEFT     = 0b0'111111111'111111111'111111111'111111111'111111111'111111111'000000000;
	static constexpr U64 MASK_RIGHT    = 0b0'000000000'111111111'111111111'111111111'111111111'111111111'111111111;
	static constexpr U64 MASK_UP       = 0b0'111111110'111111110'111111110'111111110'111111110'111111110'111111110;
	static constexpr U64 MASK_DOWN     = 0b0'011111111'011111111'011111111'011111111'011111111'011111111'011111111;
	static constexpr U64 MASK_COL_ODD  = 0b0'111111111'000000000'111111111'000000000'111111111'000000000'111111111;

	static constexpr size_t MAX_MOVES = SIZE;

	U64 occupied;
	std::array<U64, 2> types;

	static Board fromString(std::string_view str);
	std::string toString() const;
	static std::string toBitString(U64 bits);

	Score eval() const;

	void generateMoves(Board*& newBoards) const;

	Score search(Board* newBoards, size_t depth) const;
};