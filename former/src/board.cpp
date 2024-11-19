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
#include "xoshiro256.h"



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

U64 Board::lsbCol(U64 bits) {
	return bits & (~bits + MASK_BOTTOM);
}
U64 Board::toColumnMask(U64 bits) {
	U64 addShit = (bits & ~MASK_TOP) + ~MASK_TOP;
	U64 colLsb = ((addShit | bits) >> (HEIGHT - 1)) & MASK_BOTTOM;
	return (colLsb << HEIGHT) - colLsb;
}


U64 stubbornFakes = 0;
U64 stubbornHits = 0;

// still needs floodfill later to check if the move is "complete"
U64 Board::stubbornMoves() const {
	// cells that have the same color as the one above, plus not occupied cells.
	U64 stubborn = ((~(((types[0] >> 1) ^ types[0]) | ((types[1] >> 1) ^ types[1])) | ~(occupied >> 1)) | MASK_TOP);

	// wipe out lowest group until only the top one is left
	for (size_t i = 0; i < 4; i++) {
		U64 lsb = lsbCol(stubborn);
		U64 lowestRemoved = stubborn & (stubborn + lsb);
		U64 doRemoveMask = toColumnMask(lowestRemoved); // check if all the groups (aka the top one) have been removed
		stubborn = (stubborn & ~doRemoveMask) | (lowestRemoved & doRemoveMask); // if so, theres only one group left, restore it
	}
	stubborn &= occupied;

	std::array<U64, 2> stubbornTypes{ stubborn & types[0], stubborn & types[1] };
	std::array<U64, 2> typeCols { toColumnMask(stubbornTypes[0]), toColumnMask(stubbornTypes[1]) };

	// if the neighboring columns are the same color, check if any cells connect
	U64 connectsLeft = ~toColumnMask((stubbornTypes[0] ^ (stubbornTypes[0] >> HEIGHT)) | (stubbornTypes[1] ^ (stubbornTypes[1] >> HEIGHT)));
	U64 sameStubbornColorLeft = ~((typeCols[0] ^ (typeCols[0] >> HEIGHT)) | (typeCols[1] ^ (typeCols[1] >> HEIGHT)));
	U64 connectsOk = ~sameStubbornColorLeft | connectsLeft;
	connectsOk &= connectsOk << HEIGHT;
	stubborn &= connectsOk;

	// check if the -1, 0, +1 columns have any of the same color remaining
	U64 stubbornColorInSameColumn  =  ~stubborn            & ~((typeCols[0] ^  types[0]           ) | (typeCols[1] ^  types[1]           ));
	U64 stubbornColorInLeftColumn  = (~stubborn << HEIGHT) & ~((typeCols[0] ^ (types[0] << HEIGHT)) | (typeCols[1] ^ (types[1] << HEIGHT)));
	U64 stubbornColorInRightColumn = (~stubborn >> HEIGHT) & ~((typeCols[0] ^ (types[0] >> HEIGHT)) | (typeCols[1] ^ (types[1] >> HEIGHT)));
	U64 stubbornCols = ~toColumnMask(stubbornColorInSameColumn | stubbornColorInLeftColumn | stubbornColorInRightColumn);

	return stubborn & stubbornCols;

	// if (stubbornCols & MASK_ANY) {
	// 	std::cout << toString() << std::endl;
	// 	std::cout << toBitString(occupied) << std::endl;
	// 	std::cout << toBitString(stubborn) << std::endl;
	// 	std::cout << toBitString(connectsOk) << std::endl;
	// 	std::cout << toBitString(stubbornColorInRightColumn) << std::endl;
	// 	std::cout << toBitString(stubbornColorInSameColumn) << std::endl;
	// 	std::cout << toBitString(stubbornColorInLeftColumn) << std::endl;
	// 	std::cout << toBitString(stubbornCols) << std::endl;
	// 	std::cout << toBitString(stubborn & stubbornCols) << std::endl;
	// }
}

U64 Board::partialOrderReductionMask(U64 move) const {

	// std::cout << "in:   " << toBitString(move) << std::endl;
	// std::cout << "occ:  " << toBitString(occupied) << std::endl;
	// East 1: Mask away all moves starting 2 columns to the right
	auto colIndex = _tzcnt_u64(move) / HEIGHT * HEIGHT;

	// std::cout << "lsbm: " << toBitString(MASK_LEFT << colIndex) << std::endl;
	// std::cout << "mask: " << toBitString(~((~occupied & (MASK_LEFT << colIndex)) >> HEIGHT)) << std::endl;
	U64 east3 = ~((~occupied & (MASK_LEFT << colIndex)) >> HEIGHT);

	// East 2: Mask away all moves starting 1 column to the right if some color bullshit
	U64 cellsToCheck = ((0x200ULL << colIndex) - (move & -move)) >> HEIGHT;
	// std::cout << "check" << toBitString(cellsToCheck) << std::endl;
	U64 type0 = move & types[0] ? ~0ULL : 0;
	U64 type1 = move & types[1] ? ~0ULL : 0;
	if (cellsToCheck != (cellsToCheck & ((types[0] ^ type0) | (types[1] ^ type1))))
		colIndex -= HEIGHT;

	U64 result = ~0ULL << colIndex;

	// std::cout << "out1: " << toBitString(result) << std::endl;
	result &= east3;
	// std::cout << "out2: " << toBitString(result) << std::endl;

	return result;
}


U64 MurmurHash3(U64 key) {
    key ^= key >> 33;
    key *= 0xff51afd7ed558ccdULL;
    key ^= key >> 33;
    key *= 0xc4ceb9fe1a85ec53ULL;
    key ^= key >> 33;
    return key;
}
U64 Board::hash() const {
    U64 hash = 0;
    hash ^= MurmurHash3(types[0]);
    hash ^= MurmurHash3(types[1]);
    hash ^= MurmurHash3(occupied);
    return hash;
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


constexpr bool COUNT_NODES = false;
constexpr bool COUNT_TT_STATS = false;
U64 ttHits = 0;
U64 ttEntries = 0;
U64 ttMisses = 0;
U64 ttCollisions = 0;
U64 nodes = 0;

U64 lastEntries = 0;
void Board::logStats() {
	if constexpr (COUNT_TT_STATS) {
		std::cout << "tt hits: " << ttHits << " (" << std::round(ttHits * 1000 / (ttHits + ttMisses + 1)) << "‰) collisions: " << ttCollisions << " (" << std::round(ttCollisions * 1000 / (ttMisses + 1)) << "‰) entries: " << ttEntries << " (" << std::round(ttEntries * 1000 / 2 / tt->size()) << "‰) size: 2 * " << tt->size() << std::endl;
		lastEntries = ttEntries;
	}
	if constexpr (COUNT_NODES)
		std::cout << "nodes: " << nodes << std::endl;
	// std::cout << "stubborn fakes: " << stubbornFakes << " hits: " << stubbornHits << std::endl;
}


template<typename Callable>
bool Board::generateMoves(U64 moveMask, Callable cb) const {

	U64 leftSame  = occupied & ~((types[0] << HEIGHT) ^ types[0]) & ~((types[1] << HEIGHT) ^ types[1]) & ~MASK_LEFT;
	U64 rightSame = occupied & ~((types[0] >> HEIGHT) ^ types[0]) & ~((types[1] >> HEIGHT) ^ types[1]) & ~MASK_RIGHT;
	U64 upSame    = occupied & ~((types[0] << 1     ) ^ types[0]) & ~((types[1] << 1     ) ^ types[1]) & ~MASK_BOTTOM;
	U64 downSame  =            ~((types[0] >> 1     ) ^ types[0]) & ~((types[1] >> 1     ) ^ types[1]) & ~MASK_TOP;

	auto moveFromMoveBits = [&](U64& bits) -> U64 {
		U64 move = bits & -bits;
		U64 lastMove;
		// profiling: 11%
		do {
			for (size_t i = 0; i < 2; i++) { // somewhat unrolled to make branch predictor happy
				lastMove = move;
				move |= ((move << HEIGHT) & leftSame) | ((move >> HEIGHT) & rightSame) | ((move << 1) & upSame) | ((move >> 1) & downSame);
			}
		} while (lastMove != move);
		return move;
	};


	// U64 stubborn = stubbornMoves();
	// while (stubborn) [[unlikely]] {
	// 	// stubbornFakes++;
	// 	U64 move = moveFromMoveBits(stubborn);
	// 	bool isComplete = (move & stubborn) == move;
	// 	stubborn &= ~move;
	// 	if (!isComplete)
	// 		continue;
	// 	// stubbornFakes--;
	// 	// stubbornHits++;

	// 	// actually execute the stubborn move and exit
	// 	Board board = *this;
	// 	board.occupied &= ~move;
	// 	return cb(board, move);
	// }

	U64 moves = occupied & moveMask;

	while (moves) {
		U64 move = moveFromMoveBits(moves);
		moves &= ~move;

		Board board = *this;
		board.occupied &= ~move;

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

template<bool rootSearch>
std::conditional_t<rootSearch, SearchReturn, Score> Board::search(Move* newMoves, Depth depth, U64 move, U64 hash, const Board& prevBoard) const {
	if constexpr (COUNT_NODES)
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
	if (USE_TT && depth > TT_DEPTH_LIMIT && !rootSearch) {
		entry = &(*tt)[hash % tt->size()];

		if (entry->recent.board == *this && entry->recent.depth >= depth) {
			if constexpr (COUNT_TT_STATS)
				ttHits++;
			return SearchReturn{ .score = entry->recent.score };
		}
		if (entry->deep.board == *this && entry->deep.depth >= depth) {
			if constexpr (COUNT_TT_STATS)
				ttHits++;
			return SearchReturn{ .score = entry->deep.score };
		}
		if constexpr (COUNT_TT_STATS)
			ttMisses++;
	}

	auto* newMovesBegin = newMoves;
	Score best = std::numeric_limits<Score>::max() - MAX_MOVES;
	Board bestNextBoard;
	if (!generateMoves(prevBoard.partialOrderReductionMask(move), [&](Board& board, U64 move) {
		if (board.occupied == 0) {
			best = 1;
			bestNextBoard = board;
			return true;
		}
		if (depth < board.movesLowerBound())
			return false;

		newMoves->board = board;
		newMoves->move = move;
		newMoves++;

		return false;
	})) {
		// recursive search
		if (USE_TT && depth - 1 > TT_DEPTH_LIMIT)
			for (auto it = newMovesBegin; it != newMoves; ++it) {
				it->hash = it->board.hash();
				__builtin_prefetch(&(*tt)[it->hash % tt->size()]);
			}
		for (auto it = newMovesBegin; it != newMoves; ++it) {
			auto score = it->board.search<false>(newMoves, depth - 1, it->move, it->hash, *this) + 1;
			if (score < best) {
				best = score;
				bestNextBoard = it->board;
				if (score <= depth)
					break;
			}
		}
	}


	if (USE_TT && depth > TT_DEPTH_LIMIT && !rootSearch) {
		auto& replaceEntry = entry->deep.depth < depth ? entry->deep : entry->recent;
		if constexpr (COUNT_TT_STATS) {
			if (!replaceEntry.board.occupied)
				ttEntries++;
			if (replaceEntry.board.occupied && replaceEntry.board != *this)
				ttCollisions++;
		}
		replaceEntry = {
			.board = *this,
			.depth = depth,
			.score = best,
		};
	}

	if constexpr (!rootSearch)
		return best;
	else
		return SearchReturn{
			.board = bestNextBoard,
			.move = (types[0] ^ bestNextBoard.types[0]) | (types[1] ^ bestNextBoard.types[1]),
			.score = best,
		};
}

template SearchReturn Board::search<true> (Move* newMoves, Depth depth, U64 moveMask, U64 hash, const Board& prevBoard) const;
template Score        Board::search<false>(Move* newMoves, Depth depth, U64 moveMask, U64 hash, const Board& prevBoard) const;