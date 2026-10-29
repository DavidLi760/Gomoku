#ifndef GOMOKU_HPP
# define GOMOKU_HPP

# include <SFML/Graphics.hpp>
# include <array>
# include <vector>
# include <iostream>
# include "Solver.hpp"
# include "Position.hpp"
# include "UI.hpp"
# define RED "\033[31m"
# define GREEN "\033[32m"
# define YELLOW "\033[33m"
# define BLUE "\033[34m"
# define MAGENTA "\033[35m"
# define CYAN "\033[36m"
# define GRAY "\033[90m"
# define BOLD "\033[1m"
# define UNDER "\033[4m"
# define BLINK "\033[5m"
# define ERASE = "\033[2K\r"
# define RESET "\033[0m"

class Gomoku {
	public:
		static const uint8_t SIZE = 19;
		enum Cell
		{
			EMPTY = 0,
			BLACK = 1,
			WHITE = 2
		};

		Gomoku();
		~Gomoku();

		int run();
		void useAI(bool ai);
		void printBoardCount() const;

		static bool isOnBoard(int x, int y);
		static int checkWinning(const std::vector<std::vector<uint8_t>>& board,
				int player, const std::array<int, 2>& captureStone);
		static bool capture(std::vector<std::vector<uint8_t>>& board,
				int x, int y, int turn, std::array<int, 2>& captureStone,
				std::array<int, 2>* captured = nullptr);
		static bool checkDoubleFreeThree(const std::vector<std::vector<uint8_t>>& board,
				int turn, int lastX, int lastY);

	private:
		std::vector<std::vector<uint8_t>> _board;
		std::vector<double> solveTime;
		Solver _solver;
		UI _ui;

		int _turn;
		int _winner;
		bool _ai;
		bool _sugg;
		Position _suggPos;

		std::vector<History> _moveHistory;

		std::array<int, 2> _captureStone = {0, 0};
		std::array<int, 2> _winnerRecord = {0, 0};

		int _winningHandler();
		Position _gomokuSolver();
		std::vector< std::vector<uint8_t> > _updateBorad(int x, int y);
		void _reset();
};

#endif
