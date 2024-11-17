#include "board.h"

#include <cmath>
#include <iostream>
#include <vector>
#include <chrono>



int main(int argc, char** argv) {
	for (int i = 0; i < 10; i++) {
		auto board = Board::fromString("PGGOGPG GOBBPBG BOOGPBG BPOOBOG GGGBBOO GBGGOPO PGOOGPG OGPPPGO OOGPOOO");
		std::cout << board.toString() << std::endl;

		auto startTotal = std::chrono::high_resolution_clock::now();

		tt = std::make_unique<TT>();
		U64 depth = 1;
		bool foundSolution = false;
		std::string solution = "";
		while (board.occupied) {
			SearchReturn result;
			do {
				auto start = std::chrono::high_resolution_clock::now();
				result = board.search<true>(depth);
				auto end = std::chrono::high_resolution_clock::now();
				// time in milliseconds
				std::chrono::duration<double> elapsed = end - start;
				if (!foundSolution) {
					if (elapsed.count() > 0.1)
						std::cout << "depth " << depth << ": " << std::round(elapsed.count() * 1000) << "ms" << std::endl;
					// Board::logStats();
				}
				if (result.score > depth) {
					if (foundSolution)
						throw std::runtime_error("Solution lost..");
					depth++;
				} else
					foundSolution = true;
			} while (!foundSolution);

			solution += Board::toMoveString(result.move) + " ";

			board = result.board;
			depth--;
		}
		auto endTotal = std::chrono::high_resolution_clock::now();
	}

	// std::cout << "solution: " << solution << std::endl;
	// std::chrono::duration<double> elapsedTotal = endTotal - startTotal;
	// std::cout << "Total elapsed time: " << elapsedTotal.count() << " seconds" << std::endl;
	// Board::logStats();

  	return 0;
}
