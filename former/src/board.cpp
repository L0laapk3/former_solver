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
	return colLsb * MASK_LEFT;
}


U64 Board::partialOrderReductionMask(U64 move, Board& newBoard) const {
	// Partial order reductions: Mask away all moves starting 2 columns to the right
	return ~0ULL << (_tzcnt_u64(move >> HEIGHT) / HEIGHT * HEIGHT);
}

template<typename Callable>
void Board::generateMoves(U64 moveMask, Callable cb) const {
	U64 moves = occupied & moveMask;

	U64 leftSame  = ~((types[0] << HEIGHT) ^ types[0]) & ~((types[1] << HEIGHT) ^ types[1]) & ~MASK_LEFT;
	U64 rightSame = ~((types[0] >> HEIGHT) ^ types[0]) & ~((types[1] >> HEIGHT) ^ types[1]) & ~MASK_RIGHT;
	U64 upSame    = ~((types[0] << 1) ^ types[0]) & ~((types[1] << 1) ^ types[1]) & ~MASK_BOTTOM;
	U64 downSame  = ~((types[0] >> 1) ^ types[0]) & ~((types[1] >> 1) ^ types[1]) & ~MASK_TOP;

	while (moves) {
		U64 move = moves & -moves;
		U64 lastMove;
		do {
			lastMove = move;
			// it may be faster to do less iterations but longer critical path, but probably not.
			move |= occupied & (((move << HEIGHT) & leftSame) | ((move >> HEIGHT) & rightSame) | ((move << 1) & upSame) | ((move >> 1) & downSame));
		} while (lastMove != move);

		Board newBoard = *this;
		newBoard.occupied &= ~move;
		moves             &= ~move;

		toColumnMask(move);

		// std::cout << "before gravity" << std::endl;
		// std::cout << newBoard.toString() << std::endl;

		// apply gravity
		for (auto& type : newBoard.types)
			type = _pext_u64(type, newBoard.occupied);

		newBoard.occupied = _pdep_u64(_pext_u64( MASK_COL_ODD, (newBoard.occupied &  MASK_COL_ODD) | ((~newBoard.occupied &  MASK_COL_ODD) << HEIGHT)),  MASK_COL_ODD)
		                  | _pdep_u64(_pext_u64(~MASK_COL_ODD, (newBoard.occupied & ~MASK_COL_ODD) | ((~newBoard.occupied & ~MASK_COL_ODD) << HEIGHT)), ~MASK_COL_ODD);

		for (auto& type : newBoard.types)
			type = _pdep_u64(type, newBoard.occupied);

		// std::cout << "after gravity" << std::endl;
		// std::cout << newBoard.toString() << std::endl;

		if (cb(newBoard, partialOrderReductionMask(move, newBoard)))
			return;
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

template<bool rootSearch>
std::conditional_t<rootSearch, SearchReturn, Score> Board::search(Depth depth, U64 moveMask) const {
	if constexpr (countNodes)
		nodes++;
	if (occupied == 0) {
		if constexpr (!rootSearch)
			return { 0 };
		else
			return { .score = 0 };
	}

	if (depth < movesLowerBound()) {
		if constexpr (!rootSearch)
			return { std::numeric_limits<Score>::max() - MAX_MOVES };
		else
			return { .score = std::numeric_limits<Score>::max() - MAX_MOVES };
	}

	// TT lookup
	TTEntry* entry;
	if (depth > TT_DEPTH_LIMIT && !rootSearch) {
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

	// recursive search
	Score best = std::numeric_limits<Score>::max() - MAX_MOVES;
	Board bestNextBoard;
	generateMoves(moveMask, [&](Board board, U64 moveMask) {
		auto score = board.search<false>(depth - 1, moveMask) + 1;
		if (score < best) {
			best = score;
			bestNextBoard = board;
			if (score <= depth)
				return true;
		}
		return false;
	});


	if (depth > TT_DEPTH_LIMIT && (!entry->board.occupied || entry->depth < depth + 2))
		*entry = {
			.board = *this,
			.depth = depth,
			.score = best,
		};

	if constexpr (!rootSearch)
		return best;
	else
		return {
			.board = bestNextBoard,
			.move = (types[0] ^ bestNextBoard.types[0]) | (types[1] ^ bestNextBoard.types[1]) | (occupied ^ bestNextBoard.occupied),
			.score = best,
		};
}

template SearchReturn Board::search<true> (Depth depth, U64 moveMask) const;
template Score        Board::search<false>(Depth depth, U64 moveMask) const;