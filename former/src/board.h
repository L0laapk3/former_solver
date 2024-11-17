#pragma once

#include <array>
#include <string_view>
#include <string>
#include <span>
#include <iterator>

#include "types.h"

typedef U64 Score;

struct Move;
struct SearchReturn;

struct Board {
	static constexpr size_t WIDTH = 7;
	static constexpr size_t HEIGHT = 9;
	static constexpr size_t SIZE = WIDTH * HEIGHT;

	static constexpr U64 MASK_LEFT     = 0b0'000000000'000000000'000000000'000000000'000000000'000000000'111111111;
	static constexpr U64 MASK_RIGHT    = 0b0'111111111'000000000'000000000'000000000'000000000'000000000'000000000;
	static constexpr U64 MASK_BOTTOM   = 0b0'000000001'000000001'000000001'000000001'000000001'000000001'000000001;
	static constexpr U64 MASK_TOP      = 0b0'100000000'100000000'100000000'100000000'100000000'100000000'100000000;
	static constexpr U64 MASK_COL_ODD  = 0b0'111111111'000000000'111111111'000000000'111111111'000000000'111111111;

	static constexpr size_t MAX_MOVES = SIZE;

	U64 occupied;
	std::array<U64, 2> types;

	static Board fromString(std::string_view str);
	std::string toString() const;
	static std::string toBitString(U64 bits);
	static std::string toMoveString(U64 move);

	static U64 toColumnMask(U64 bits);

	Score eval() const;

	void generateMoves(Move*& newMoves, U64 moveMask = ~0ULL) const;

	template<bool returnMove>
	std::conditional_t<returnMove, SearchReturn, Score> search(Move* newMoves, size_t depth, U64 moveMask = ~0ULL) const;
};

struct SearchReturn {
	Score score;
	Board board;
	U64 move;
};

struct Move {
	Board board;
	U64 moveMask;
};