#pragma once

#include <array>
#include <string_view>
#include <string>
#include <span>
#include <iterator>

#include "types.h"

struct Board;
struct Board {
	static constexpr size_t WIDTH = 7;
	static constexpr size_t HEIGHT = 9;
	static constexpr size_t SIZE = WIDTH * HEIGHT;

	static constexpr U64 MASK_LEFT  = 0b0'1111110'1111110'1111110'1111110'1111110'1111110'1111110'1111110'1111110;
	static constexpr U64 MASK_RIGHT = 0b0'0111111'0111111'0111111'0111111'0111111'0111111'0111111'0111111'0111111;
	static constexpr U64 MASK_UP    = 0b0'1111111'1111111'1111111'1111111'1111111'1111111'1111111'1111111'0000000;
	static constexpr U64 MASK_DOWN  = 0b0'0000000'1111111'1111111'1111111'1111111'1111111'1111111'1111111'1111111;

	static constexpr size_t MAX_MOVES = SIZE;

	U64 occupied;
	std::array<U64, 2> type;

	static Board fromString(std::string_view str);
	std::string toString() const;

	void generateMoves(Board*& newBoards) const;

	void search(Board* newBoards, size_t depth) const;
};