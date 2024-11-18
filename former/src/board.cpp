#include "board.h"
#include "former/src/types.h"
#include <bitset>
#include <cmath>
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


U64 Board::partialOrderReductionMask(U64 move, Board& board) const {
	// East 1: Mask away all moves starting 2 columns to the right
	auto colIndex = _tzcnt_u64(move) / HEIGHT * HEIGHT;

	// East 2: Mask away all moves starting 1 column to the right if some color bullshit
	U64 cellsToCheck = ((0x200ULL << colIndex) - (move & -move)) >> HEIGHT;
	// std::cout << "check" << toBitString(cellsToCheck) << std::endl;
	U64 type0 = move & types[0] ? ~0ULL : 0;
	U64 type1 = move & types[1] ? ~0ULL : 0;
	// std::cout << "type0: " << (type0 ? "1" : "0") << " type1: " << (type1 ? "1" : "0") << " cond: " << (cellsToCheck != (cellsToCheck & ((types[0] ^ type0) | (types[1] ^ type1))) ? "1" : "0") << std::endl;
	if (cellsToCheck != (cellsToCheck & ((types[0] ^ type0) | (types[1] ^ type1))))
		colIndex -= HEIGHT;

	return ~0ULL << colIndex;
}

U64 Board::hash() const {
	U64 hash = 0;
	hash ^= types[0];
	hash *= 0x9e3779b97f4a7c15;
	hash ^= types[0] >> 21;
	hash *= 0x9e3779b97f4a7c15;
	hash ^= types[0] >> 42;
	hash *= 0x9e3779b97f4a7c15;
	hash ^= types[1];
	hash *= 0x9e3779b97f4a7c15;
	hash ^= types[1] >> 21;
	hash *= 0x9e3779b97f4a7c15;
	hash ^= types[1] >> 42;
	hash *= 0x9e3779b97f4a7c15;
	hash ^= occupied;
	hash *= 0x9e3779b97f4a7c15;
	hash ^= occupied >> 21;
	hash *= 0x9e3779b97f4a7c15;
	hash ^= occupied >> 42;
	return hash;

	U64 hash1 = ((U128)types[0] * 12769894017520768087ULL) >> 64;
	U64 hash2 = ((U128)types[1] * 14976091711589288359ULL) >> 64;
	U64 hash3 = ((U128)occupied * 9292276211755231913ULL) >> 64;
	return std::hash<U64>{}(hash1 ^ hash2 ^ hash3);
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

constexpr bool countNodes = true;
constexpr bool collectTTStats = true;
U64 ttHits = 0;
U64 ttCollisions = 0;
U64 ttEmpty = 0;
U64 nodes = 0;

void Board::logStats() {
	if constexpr (collectTTStats) {
		// std::cout << "tt hits: " << ttHits << " (" << std::round(ttHits * 1000 / (ttHits + ttEmpty + 1)) << "‰) collisions: " << ttCollisions << " (" << std::round(ttCollisions * 1000 / (ttHits + 1)) << "‰) usage: " << std::round(ttEmpty * 1000 / tt->size()) << "‰" << std::endl;
		std::cout << "tt hits: " << ttHits << " (" << std::round(ttHits * 1000 / (ttHits + ttCollisions + ttEmpty + 1)) << "‰) collisions: " << ttCollisions << " (" << std::round(ttCollisions * 1000 / (ttHits + 1)) << "‰) usage: " << std::round(ttEmpty * 1000 / 2 / tt->size()) << "‰ size: 2 * " << tt->size() << std::endl;
		ttHits = 0;
		ttCollisions = 0;
	}
	if constexpr (countNodes)
		std::cout << "nodes: " << nodes << std::endl;
}

template<typename Callable>
bool Board::generateMoves(U64 moveMask, Callable cb) const {
	U64 moves = occupied & moveMask;

	U64 leftSame  = occupied & ~((types[0] << HEIGHT) ^ types[0]) & ~((types[1] << HEIGHT) ^ types[1]) & ~MASK_LEFT;
	U64 rightSame = occupied & ~((types[0] >> HEIGHT) ^ types[0]) & ~((types[1] >> HEIGHT) ^ types[1]) & ~MASK_RIGHT;
	U64 upSame    = occupied & ~((types[0] << 1     ) ^ types[0]) & ~((types[1] << 1     ) ^ types[1]) & ~MASK_BOTTOM;
	U64 downSame  =            ~((types[0] >> 1     ) ^ types[0]) & ~((types[1] >> 1     ) ^ types[1]) & ~MASK_TOP;

	while (moves) {
		U64 move = moves & -moves;
		U64 lastMove;
		// profiling: 11%
		do {
			for (size_t i = 0; i < 2; i++) { // somewhat unrolled to make branch predictor happy
				lastMove = move;
				move |= ((move << HEIGHT) & leftSame) | ((move >> HEIGHT) & rightSame) | ((move << 1) & upSame) | ((move >> 1) & downSame);
			}
		} while (lastMove != move);

		Board board = *this;
		board.occupied &= ~move;
		moves          &= ~move;

		// std::cout << "before gravity" << std::endl;
		// std::cout << newBoard.toString() << std::endl;

		// apply gravity
		for (auto& type : board.types)
			type = _pext_u64(type, board.occupied);

		// profiling: 8%
		board.occupied = _pdep_u64(_pext_u64( MASK_COL_ODD, (board.occupied &  MASK_COL_ODD) | ((~board.occupied &  MASK_COL_ODD) << HEIGHT)),  MASK_COL_ODD)
		               | _pdep_u64(_pext_u64(~MASK_COL_ODD, (board.occupied & ~MASK_COL_ODD) | ((~board.occupied & ~MASK_COL_ODD) << HEIGHT)), ~MASK_COL_ODD);

		for (auto& type : board.types)
			type = _pdep_u64(type, board.occupied);

		// std::cout << "after gravity" << std::endl;
		// std::cout << toString() << std::endl;

		// std::cout << "in:  " << toBitString(move) << std::endl;
		if (cb(board, move))
			return true;
		// std::cout << "out: " << toBitString(newMoves->moveMask) << std::endl;
		// std::cout << board.toString() << std::endl;
	}
	return false;
}

template<bool returnMove>
std::conditional_t<returnMove, SearchReturn, Score> Board::search(Move* newMoves, Depth depth, U64 moveMask) const {
	if constexpr (countNodes)
		nodes++;
	if (occupied == 0) {
		return SearchReturn{ .score = 0 };
	}

	if (depth < movesLowerBound()) {
		return SearchReturn{ .score = std::numeric_limits<Score>::max() - MAX_MOVES };
	}

	// TT lookup
	// profiling: 45%
	TTRow* entry;
	if (depth > TT_DEPTH_LIMIT && !returnMove) {
		auto hash = this->hash();
		entry = &(*tt)[hash % tt->size()];
		if constexpr (collectTTStats) {
			if (entry->recent.board.occupied == 0)
				ttEmpty++;
			else if (entry->recent.board == *this || entry->deep.board == *this)
				ttHits++;
			else
				ttCollisions++;
		}

		if (entry->recent.board == *this && entry->recent.depth >= depth)
			return SearchReturn{ .score = entry->recent.score };
		if (entry->deep.board == *this && entry->deep.depth >= depth)
			return SearchReturn{ .score = entry->deep.score };
	}

	auto* newMovesBegin = newMoves;
	Score best = std::numeric_limits<Score>::max() - MAX_MOVES;
	Board bestNextBoard;
	if (!generateMoves(moveMask, [&](Board& board, U64 move) {
		if (board.occupied == 0) {
			best = 1;
			bestNextBoard = board;
			return true;
		}
		if (depth < board.movesLowerBound())
			return false;

		*newMoves = {
			.board = board,
			.moveMask = partialOrderReductionMask(move, board),
		};
		newMoves++;

		return false;
	})) {
		// recursive search
		for (auto it = newMovesBegin; it != newMoves; ++it) {
			auto hash = it->board.hash();
			__builtin_prefetch(&(*tt)[hash % tt->size()]);
		}
		for (auto it = newMovesBegin; it != newMoves; ++it) {
			auto score = it->board.search<false>(newMoves, depth - 1, it->moveMask) + 1;
			if (score < best) {
				best = score;
				bestNextBoard = it->board;
				if (score <= depth)
					break;
			}
		}
	}


	if (depth > TT_DEPTH_LIMIT && !returnMove) {
		TTEntry newEntry{
			.board = *this,
			.depth = depth,
			.score = best,
		};
		if (entry->deep.depth < depth)
			entry->deep = newEntry;
		else
			entry->recent = newEntry;
	}

	if constexpr (!returnMove)
		return best;
	else
		return SearchReturn{
			.board = bestNextBoard,
			.move = (types[0] ^ bestNextBoard.types[0]) | (types[1] ^ bestNextBoard.types[1]),
			.score = best,
		};
}

template SearchReturn Board::search<true> (Move* newMoves, Depth depth, U64 moveMask) const;
template Score        Board::search<false>(Move* newMoves, Depth depth, U64 moveMask) const;