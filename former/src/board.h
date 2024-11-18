#pragma once

#include <array>
#include <string_view>
#include <string>
#include <span>
#include <iterator>
#include <memory>

#include "types.h"

typedef U32 Score;
typedef U32 Depth;

struct Move;
struct SearchReturn;

struct Board {
	static constexpr size_t WIDTH = 7;
	static constexpr size_t HEIGHT = 9;
	static constexpr size_t SIZE = WIDTH * HEIGHT;

	static constexpr U64 MASK_ANY      = 0b0'111111111'111111111'111111111'111111111'111111111'111111111'111111111;
	static constexpr U64 MASK_LEFT     = 0b0'000000000'000000000'000000000'000000000'000000000'000000000'111111111;
	static constexpr U64 MASK_RIGHT    = 0b0'111111111'000000000'000000000'000000000'000000000'000000000'000000000;
	static constexpr U64 MASK_BOTTOM   = 0b0'000000001'000000001'000000001'000000001'000000001'000000001'000000001;
	static constexpr U64 MASK_TOP      = 0b0'100000000'100000000'100000000'100000000'100000000'100000000'100000000;
	static constexpr U64 MASK_COL_ODD  = 0b0'111111111'000000000'111111111'000000000'111111111'000000000'111111111;

	static constexpr size_t MAX_MOVES = SIZE;

	U64 occupied;
	std::array<U64, 2> types;

	auto operator<=>(const Board&) const = default;

	static Board fromString(std::string_view str);
	std::string toString() const;
	static std::string toBitString(U64 bits);
	static std::string toMoveString(U64 move);

	static U64 toColumnMask(U64 bits);

	U64 hash() const;
	static void logStats();

	Score movesLowerBound() const;

	U64 partialOrderReductionMask(U64 move, Board& board) const;
	template<typename Callable>
	bool generateMoves(U64 moveMask, Callable cb) const;

	template<bool rootSearch>
	std::conditional_t<rootSearch, SearchReturn, Score> search(Move* newMoves, Depth depth, U64 moveMask = ~0ULL, U64 hash = 0ULL) const;
};

struct alignas(32) TTEntry {
	Board board;
	Depth depth;
	Score score;
};
struct alignas(64) TTRow {
	TTEntry deep;
	TTEntry recent;
};

constexpr U64 TT_DEPTH_LIMIT = 0;
typedef std::array<TTRow, 32ULL * 1024 * 1024 * 1024 / 8 / sizeof(TTRow)> TT;
extern std::unique_ptr<TT> tt;

struct SearchReturn {
	Board board;
	U64 move;
	Score score;

	operator Score() const { return score; }
};

struct Move {
	Board board;
	U64 moveMask;
	U64 hash;
};