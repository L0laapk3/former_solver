#include "board.h"

#include <iostream>
#include <vector>
#include <chrono>


std::vector<Move> newMoves = std::vector<Move>(Board::MAX_MOVES * Board::SIZE);



int main(int argc, char** argv) {
	auto board = Board::fromString("PGGOGPG GOBBPBG BOOGPBG BPOOBOG GGGBBOO GBGGOPO PGOOGPG OGPPPGO OOGPOOO");
	std::cout << board.toString() << std::endl;

	auto start = std::chrono::high_resolution_clock::now();

	if constexpr (0) {
		Move* newMovesEnd = &newMoves[0];
		board.generateMoves(newMovesEnd);

		std::cout << "move count: " << newMovesEnd - &newMoves[0] << std::endl;
		if constexpr (0)
			for (auto it = &newMoves[0]; it != newMovesEnd; ++it) {
				std::cout << it->board.toString() << std::endl;
			}
	} else {
		U64 depth = 1;
		bool foundSolution = false;
		std::string solution = "";
		while (board.occupied) {
			SearchReturn result;
			do {
				result = board.search<true>(&newMoves[0], depth);
				if (result.score > depth) {
					std::cout << "No solution at depth " << depth << std::endl;
					if (foundSolution)
						return 1;
					depth++;
				}
			} while (!foundSolution);
			std::cout << Board::toMoveString(result.move) << std::endl;
			std::cout << result.board.toString() << std::endl;
			// std::cout << Board::toBitString(result.board.types[0]) << std::endl;
			// std::cout << Board::toBitString(result.board.types[1]) << std::endl;
			// std::cout << Board::toBitString(result.board.occupied) << std::endl;


			solution += Board::toMoveString(result.move) + " ";

			board = result.board;
			depth--;
		}

		std::cout << "solution: " << solution << std::endl;
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "Elapsed time: " << elapsed.count() << " seconds" << std::endl;

  	return 0;
}
