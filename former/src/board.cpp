#include "board.h"
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


U64 Board::toColumnMask(U64 bits) {
	U64 addShit = (bits & ~MASK_TOP) + ~MASK_TOP;
	U64 colLsb = ((addShit | bits) >> (HEIGHT - 1)) & MASK_BOTTOM;
	// This multiplication could be replaced with a subtraction and some masking/oring?
	return (colLsb & MASK_BOTTOM) * MASK_LEFT;
}



void Board::generateMoves(Move*& newMoves, U64 moveMask) const {
	U64 moves = occupied & moveMask;

	U64 leftSame  = ~((types[0] << HEIGHT) ^ types[0]) & ~((types[1] << HEIGHT) ^ types[1]) & ~MASK_LEFT;
	U64 rightSame = ~((types[0] >> HEIGHT) ^ types[0]) & ~((types[1] >> HEIGHT) ^ types[1]) & ~MASK_RIGHT;
	U64 upSame    = ~((types[0] << 1) ^ types[0]) & ~((types[1] << 1) ^ types[1]) & ~MASK_BOTTOM;
	U64 downSame  = ~((types[0] >> 1) ^ types[0]) & ~((types[1] >> 1) ^ types[1]) & ~MASK_TOP;

	Move* newMovesBegin = newMoves;
	while (moves) {
		assert(newMoves != newMovesBegin + MAX_MOVES);
		U64 move = moves & -moves;
		U64 lastMove;
		do {
			lastMove = move;
			// it may be faster to do less iterations but longer critical path, but probably not.
			move |= ((move << HEIGHT) & leftSame) | ((move >> HEIGHT) & rightSame) | ((move << 1) & upSame) | ((move >> 1) & downSame));
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

		*newMoves = {
			.board = newBoard,
			.moveMask = ~0ULL << (_tzcnt_u64(move >> HEIGHT) / HEIGHT * HEIGHT), // Partial order reductions: Mask away all moves starting 2 columns to the right
		};

		newMoves++;
	}
}


// minimum move count required to clear the board
Score Board::eval() const {
	return std::popcount(occupied);
}


Score Board::search(Move* newMoves, size_t depth, U64 moveMask) const {
	if (occupied == 0)
		return 0;

	if (depth == 0) {
		newMoves++;
		return eval();
	}

	auto* newMovesBegin = newMoves;
	generateMoves(newMoves, moveMask);

	// // move ordering
	// std::sort(newMovesBegin, newMoves, [](const auto& a, const auto& b) {
	// 	return a.board.eval() < b.board.eval();
	// });

	// recursive search
	Score best = std::numeric_limits<Score>::max() - MAX_MOVES;
	for (auto it = newMovesBegin; it != newMoves; ++it) {
		auto score = it->board.search(newMoves, depth - 1, it->moveMask);
		if (score < 5)
			it->board.search(newMoves, depth - 1, it->moveMask);
		if (score < best)
			best = score;
	}
	return best + 1;
}