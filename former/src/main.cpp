#include "board.h"

#include <iostream>


std::array<Board, Board::MAX_MOVES * Board::SIZE> newBoards;



int main(int argc, char** argv) {
	auto board = Board::fromString("PGGOGPG GOBBPBG BOOGPBG BPOOBOG GGGBBOO GBGGOPO PGOOGPG OGPPPGO OOGPOOO");
	std::cout << board.toString() << std::endl;


	auto newBoardsEnd = newBoards.begin();
	board.generateMoves(newBoardsEnd);

	std::cout << "move count: " << newBoardsEnd - &newBoards[0] << std::endl;
	for (auto it = &newBoards[0]; it != newBoardsEnd; ++it) {
		std::cout << it->toString() << std::endl;
	}

	// std::cout << board.search(&newBoards[0], 3) << std::endl;

  	return 0;
}
