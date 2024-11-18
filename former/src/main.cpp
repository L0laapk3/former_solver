#include "board.h"

#include <cmath>
#include <iostream>
#include <vector>
#include <chrono>


std::vector<Move> newMoves = std::vector<Move>(Board::MAX_MOVES * Board::SIZE);



int main(int argc, char** argv) {
	// auto board =	Board::fromString("PGGOGPG GOBBPBG BOOGPBG BPOOBOG GGGBBOO GBGGOPO PGOOGPG OGPPPGO OOGPOOO"); // 17/11/2024
	auto board =	Board::fromString("GGGBOBB OBGPBPP BBOBOOB GPBGOBB OBBOPBO OBBOGBO BGOBOPG GOGPOGO OGGOGGO"); // 18/11/2024
	std::cout << board.toString() << std::endl;

	auto startTotal = std::chrono::high_resolution_clock::now();


	tt = std::make_unique<TT>();
	U64 maxDepth, depth = 1;
	bool foundSolution = false;
	std::string solution = "";
	while (board.occupied) {
		SearchReturn result;
		do {
			auto start = std::chrono::high_resolution_clock::now();
			result = board.search<true>(&newMoves[0], depth, 0, board);
			auto end = std::chrono::high_resolution_clock::now();
			// time in milliseconds
			std::chrono::duration<double> elapsed = end - start;
			if (!foundSolution) {
				if (elapsed.count() > 0.1)
					std::cout << "depth " << depth << ": " << std::round(elapsed.count() * 1000) << "ms" << std::endl;
				Board::logStats();
			}
			if (result.score > depth) {
				if (foundSolution)
					throw std::runtime_error("Solution lost..");
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