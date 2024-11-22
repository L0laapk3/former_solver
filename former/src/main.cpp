#include "board.h"

#include <cmath>
#include <iostream>
#include <vector>
#include <chrono>




int main(int argc, char** argv) {
	// auto board =	Board::fromString("PGGOGPG GOBBPBG BOOGPBG BPOOBOG GGGBBOO GBGGOPO PGOOGPG OGPPPGO OOGPOOO"); // 17/11/2024
	// auto board =	Board::fromString("GGGBOBB OBGPBPP BBOBOOB GPBGOBB OBBOPBO OBBOGBO BGOBOPG GOGPOGO OGGOGGO"); // 18/11/2024
	// auto board =	Board::fromString("GGBOGPB OPPOGGB PGPGPPO GBPOOGP OBGBOBP PGPGOOG GGGGGPO OPGPBBO BOGPOBP"); // 19/11/2024
	// auto board =	Board::fromString("BBGBBPP GGGOOBG OGPGGPO PBBOOBG BOOGPBG BOBGOBG GGOBGPG GPBBOOO GBBOPGO"); // 20/11/2024
	auto board =	Board::fromString("OOOOBPO PGGGBGO GOOPBOB PBPBBBP GOGOPOO BPPBGOP BOPGGPB GGBOGPP OPGBOPO"); // 21/11/2024
	// auto board =	Board::fromString("BPBGOGO OOPOPGB POPOOGO PBPPGPO PBOBOPB BBOPOBB BOBPPOB POPPPBG OOGPGOG"); // 22/11/2024
	std::cout << board.toString() << std::endl;

	auto startTotal = std::chrono::high_resolution_clock::now();


	if (1) { // perft
		auto newMoves = std::vector<Move>(Board::MAX_MOVES * Board::SIZE);
		auto start = std::chrono::high_resolution_clock::now();
		board.search<false, Board::NO_MULTITHREAD>(newMoves.data(), 6, ~0ULL, 0, board, 0);
		auto end = std::chrono::high_resolution_clock::now();
		U64 nodes = Board::logStats();
		std::chrono::duration<double> elapsed = end - start;
		std::cout << "perft: " << nodes / elapsed.count() / 1000000 << "M/s" << std::endl;
		exit(0);
	}


	tt = std::make_unique<TT>();
	U64 maxDepth, depth = 6;
	bool foundSolution = false;
	std::string solution = "";
	while (board.occupied) {
		SearchReturn result;
		do {
			auto start = std::chrono::high_resolution_clock::now();
			result = board.searchMT(depth, 3);
			auto end = std::chrono::high_resolution_clock::now();
			// time in milliseconds
			std::chrono::duration<double> elapsed = end - start;
			if (!foundSolution) {
				if (elapsed.count() > 0.1 || true) {
					std::cout << "depth " << depth << ": " << std::round(elapsed.count() * 1000) << "ms" << std::endl;
					Board::logStats();
				}
			}
			if (result.score > depth) {
				if (foundSolution)
					throw std::runtime_error("Shits fucked");
				depth++;
				// break;
			} else {
				if (!foundSolution)
					maxDepth = depth;
				foundSolution = true;
			}
		} while (!foundSolution);

		solution += Board::toMoveString(result.move) + " ";

		board = result.board;
		// std::cout << board.toString() << std::endl;
		depth--;
	}
	auto endTotal = std::chrono::high_resolution_clock::now();

	std::cout << "depth " << maxDepth << " solution: " << solution << std::endl;
	std::chrono::duration<double> elapsedTotal = endTotal - startTotal;
	std::cout << "Total elapsed time: " << elapsedTotal.count() << " seconds" << std::endl;
	Board::logStats();

  	return 0;
}