#include "board.h"
#include "former/src/types.h"
#include <bitset>
#include <immintrin.h>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <bit>
#include <algorithm>
#include <limits>



Board Board::fromString(std::string_view str) {
	Board board{};
	auto it = str.begin();
	for (size_t y = 0; y < HEIGHT; y++)
		for (size_t x = 0; x < WIDTH; x++) {
			U64 bit = 1ULL << (x * HEIGHT + (HEIGHT - 1 - y));
			while (true) {
				if (it == str.end()) {
					throw std::invalid_argument("invalid board string");
				} else if (*it == 'O') {

				} else if (*it == 'P') {
					board.types[0] |= bit;
				} else if (*it == 'B') {
					board.types[1] |= bit;
				} else if (*it == 'G') {
					board.types[0] |= bit;
					board.types[1] |= bit;
				} else {
					++it;
					continue;
				}
				break;
			}
			board.occupied |= bit;
			++it;
		}
	return board;
}
std::string Board::toString() const {
	std::string str;
	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
			U64 bit = 1ULL << (x * HEIGHT + (HEIGHT - 1 - y));
			if (occupied & bit) {
				if (types[0] & bit) {
					if (types[1] & bit) {
						str += "\x1b[38;5;42mG";
					} else {
						str += "\x1b[38;5;201mP";
					}
				} else {
					if (types[1] & bit) {
						str += "\x1b[38;5;27mB";
					} else {
						str += "\x1b[38;5;208mO";
					}
				}
			} else {
				str += " ";
			}
		}
		str += "\x1b[38;5;255m\n";
	}
	return str;
}
std::string Board::toBitString(U64 bits) {
	std::string result = "0b" + std::bitset<1>(bits >> 63).to_string();
	for (size_t i = 63; i > 0;) {
		 i -= 9;
		result += "'" + std::bitset<9>(bits >> i).to_string();
	}
	return result;
}
std::string Board::toMoveString(U64 move) {
	// chess notation
	U64 bit = move & -move;
	size_t x = _tzcnt_u64(bit) / HEIGHT;
	size_t y = HEIGHT - 1 - _tzcnt_u64(bit) % HEIGHT;
	return std::string(1, 'A' + x) + std::to_string(HEIGHT - y);
}


U64 Board::toColumnMask(U64 bits) {
	U64 addShit = (bits & ~MASK_TOP) + ~MASK_TOP;
	U64 colLsb = ((addShit | bits) >> (HEIGHT - 1)) & MASK_BOTTOM;
	// This multiplication could be replaced with a subtraction and some masking/oring?
	return (colLsb & MASK_BOTTOM) * MASK_LEFT;
}


U64 Board::partialOrderReductionMask(U64 move, Board& newBoard) const {
	// Partial order reductions: Mask away all moves starting 2 columns to the right
	return ~0ULL << (_tzcnt_u64(move >> HEIGHT) / HEIGHT * HEIGHT);
}

void Board::generateMoves(Move*& newMoves, U64 moveMask) const {
	U64 moves = occupied & moveMask;

	U64 leftSame  = occupied & ~((types[0] << HEIGHT) ^ types[0]) & ~((types[1] << HEIGHT) ^ types[1]) & ~MASK_LEFT;
	U64 rightSame = occupied & ~((types[0] >> HEIGHT) ^ types[0]) & ~((types[1] >> HEIGHT) ^ types[1]) & ~MASK_RIGHT;
	U64 upSame    = occupied & ~((types[0] << 1     ) ^ types[0]) & ~((types[1] << 1     ) ^ types[1]) & ~MASK_BOTTOM;
	U64 downSame  = occupied & ~((types[0] >> 1     ) ^ types[0]) & ~((types[1] >> 1     ) ^ types[1]) & ~MASK_TOP;

	Move* newMovesBegin = newMoves;
	while (moves) {
		assert(newMoves != newMovesBegin + MAX_MOVES);
		U64 move = moves & -moves;
		U64 lastMove;
		// profiling: 13%
		do {
			for (size_t i = 0; i < 2; i++) {
				lastMove = move;
				move |= ((move << HEIGHT) & leftSame) | ((move >> HEIGHT) & rightSame) | ((move << 1) & upSame) | ((move >> 1) & downSame);
			}
		} while (lastMove != move);

		Board newBoard = *this;
		newBoard.occupied &= ~move;
		moves             &= ~move;

		// std::cout << "before gravity" << std::endl;
		// std::cout << newBoard.toString() << std::endl;

		// apply gravity
		for (auto& type : newBoard.types)
			type = _pext_u64(type, newBoard.occupied);

		// profiling: 8%
		newBoard.occupied = _pdep_u64(_pext_u64( MASK_COL_ODD, (newBoard.occupied &  MASK_COL_ODD) | ((~newBoard.occupied &  MASK_COL_ODD) << HEIGHT)),  MASK_COL_ODD)
		                  | _pdep_u64(_pext_u64(~MASK_COL_ODD, (newBoard.occupied & ~MASK_COL_ODD) | ((~newBoard.occupied & ~MASK_COL_ODD) << HEIGHT)), ~MASK_COL_ODD);

		for (auto& type : newBoard.types)
			type = _pdep_u64(type, newBoard.occupied);

		// std::cout << "after gravity" << std::endl;
		// std::cout << newBoard.toString() << std::endl;

		*newMoves = {
			.board = newBoard,
			.moveMask = partialOrderReductionMask(move, newBoard),
		};

		newMoves++;
	}
}

U64 Board::hash() const {
	U64 hash1 = ((U128)types[0] * 12769894017520768087ULL) >> 64;
	U64 hash2 = ((U128)types[1] * 14976091711589288359ULL) >> 64;
	U64 hash3 = ((U128)occupied * 9292276211755231913ULL) >> 64;
	return hash1 ^ hash2 ^ hash3;
}


std::unique_ptr<TT> tt;

// minimum move count required to clear the board
Score Board::movesLowerBound() const {
	// profiling: 10%
	U64 counts = 0;
	U64 color0Cols = toColumnMask(~types[0] & ~types[1] & occupied);
	counts |= (color0Cols & ~(color0Cols << 1));
	U64 color1Cols = toColumnMask( types[0] & ~types[1]);
	counts |= (color1Cols & ~(color1Cols << 1)) << 1;
	U64 color2Cols = toColumnMask(~types[0] &  types[1]);
	counts |= (color2Cols & ~(color2Cols << 1)) << 2;
	U64 color3Cols = toColumnMask( types[0] &  types[1]);
	counts |= (color3Cols & ~(color3Cols << 1)) << 3;
	return std::popcount(counts);
}

constexpr bool countNodes = false;
constexpr bool collectTTStats = false;
U64 ttHits = 0;
U64 ttCollisions = 0;
U64 ttEmpty = 0;
U64 nodes = 0;

void Board::logStats() {
	if constexpr (collectTTStats) {
		std::cout << "tt hits: " << ttHits << " collisions: " << ttCollisions << " empty: " << ttEmpty << std::endl;
		ttHits = 0;
		ttCollisions = 0;
		ttEmpty = 0;
	}
	if constexpr (countNodes)
		std::cout << "nodes: " << nodes << std::endl;
}

template<bool returnMove>
std::conditional_t<returnMove, SearchReturn, Score> Board::search(Move* newMoves, Depth depth, U64 moveMask) const {
	if constexpr (countNodes)
		nodes++;
	if (occupied == 0) {
		if constexpr (!returnMove)
			return { 0 };
		else
			return { .score = 0 };
	}

	if (depth < movesLowerBound()) {
		if constexpr (!returnMove)
			return { std::numeric_limits<Score>::max() - MAX_MOVES };
		else
			return { .score = std::numeric_limits<Score>::max() - MAX_MOVES };
	}

	// TT lookup
	// profiling: 45%
	TTEntry* entry;
	if (depth > TT_DEPTH_LIMIT && !returnMove) {
		auto hash = this->hash();
		entry = &(*tt)[hash & (tt->size() - 1)];
		if constexpr (collectTTStats) {
			if (entry->board.occupied == 0)
				ttEmpty++;
			else if (entry->board == *this)
				ttHits++;
			else
				ttCollisions++;
		}

		if (entry->board == *this && entry->depth >= depth)
			return { entry->score };
	}

	auto* newMovesBegin = newMoves;
	generateMoves(newMoves, moveMask);

	// // move ordering
	// std::sort(newMovesBegin, newMoves, [](const auto& a, const auto& b) {
	// 	return a.board.eval() < b.board.eval();
	// });

	// recursive search
	Score best = std::numeric_limits<Score>::max() - MAX_MOVES;
	Board bestNextBoard;
	for (auto it = newMovesBegin; it != newMoves; ++it) {
		auto score = it->board.search<false>(newMoves, depth - 1, it->moveMask) + 1;
		if (score < best) {
			best = score;
			bestNextBoard = it->board;
			if (score <= depth)
				break;
		}
	}

	if (depth > TT_DEPTH_LIMIT && (!entry->board.occupied || entry->depth < depth + 2))
		*entry = {
			.board = *this,
			.depth = depth,
			.score = best,
		};

	if constexpr (!returnMove)
		return best;
	else
		return {
			.board = bestNextBoard,
			.move = (types[0] ^ bestNextBoard.types[0]) | (types[1] ^ bestNextBoard.types[1]),
			.score = best,
		};
}

template SearchReturn Board::search<true> (Move* newMoves, Depth depth, U64 moveMask) const;
template Score        Board::search<false>(Move* newMoves, Depth depth, U64 moveMask) const;