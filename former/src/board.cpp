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




void Board::generateMoves(Board*& newBoards) const {
	U64 moves = occupied;

	U64 leftSame  = ~((types[0] << HEIGHT) ^ types[0]) & ~((types[1] << HEIGHT) ^ types[1]) & MASK_LEFT;
	U64 rightSame = ~((types[0] >> HEIGHT) ^ types[0]) & ~((types[1] >> HEIGHT) ^ types[1]) & MASK_RIGHT;
	U64 upSame    = ~((types[0] << 1) ^ types[0]) & ~((types[1] << 1) ^ types[1]) & MASK_UP;
	U64 downSame  = ~((types[0] >> 1) ^ types[0]) & ~((types[1] >> 1) ^ types[1]) & MASK_DOWN;

	Board* newBoardsBegin = newBoards;
	while (moves) {
		assert(newBoards != newBoardsBegin + MAX_MOVES);
		U64 move = moves & -moves;
		U64 lastMove;
		do {
			lastMove = move;
			// it may be faster to do less iterations but longer critical path, but probably not.
			move |= ((move << HEIGHT) & leftSame) | ((move >> HEIGHT) & rightSame) | ((move << 1) & upSame) | ((move >> 1) & downSame);
		} while (lastMove != move);

		*newBoards = *this;
		newBoards->occupied ^= move;
		moves ^= move;

		std::cout << "before gravity" << std::endl;
		std::cout << newBoards->toString() << std::endl;

		// apply gravity
		for (auto& type : newBoards->types)
			type = _pext_u64(type, newBoards->occupied);

		newBoards->occupied = _pdep_u64(_pext_u64( MASK_COL_ODD, (newBoards->occupied &  MASK_COL_ODD) | ((~newBoards->occupied &  MASK_COL_ODD) << HEIGHT)),  MASK_COL_ODD)
		                    | _pdep_u64(_pext_u64(~MASK_COL_ODD, (newBoards->occupied & ~MASK_COL_ODD) | ((~newBoards->occupied & ~MASK_COL_ODD) << HEIGHT)), ~MASK_COL_ODD);

		for (auto& type : newBoards->types)
			type = _pdep_u64(type, newBoards->occupied);

		std::cout << "after gravity" << std::endl;
		std::cout << newBoards->toString() << std::endl;

		newBoards++;
	}
}



Score Board::eval() const {
	return std::popcount(occupied);
}


Score Board::search(Board* newBoards, size_t depth) const {
	if (occupied == 0)
		return 0;

	if (depth == 0) {
		*newBoards = *this;
		newBoards++;
		return eval();
	}

	Board* newBoardsBegin = newBoards;
	generateMoves(newBoards);

	// move ordering
	std::sort(newBoardsBegin, newBoards, [](const Board& a, const Board& b) {
		return a.eval() < b.eval();
	});

	// recursive search
	Score best = std::numeric_limits<Score>::max();
	for (auto it = newBoardsBegin; it != newBoards; ++it) {
		auto score = it->search(newBoards, depth - 1);
		if (score < best)
			best = score;
	}
	return best + 1;
}