# include "Gomoku.hpp"
# include <cmath>
# include "Rules.hpp"
# include "Solver.hpp"

Gomoku::Gomoku() : _board(SIZE, std::vector<uint8_t>(SIZE)), _ui(UI()), _turn(1), _winner(0), _ai(false), _sugg(false), _suggPos(Position({-1, -1})), _captureStone({0, 0}), _winnerRecord({0, 0})
{
	_solver = Solver();
}

Gomoku::~Gomoku(){}

void Gomoku::printBoardCount() const
{
	int whitecnt = 0, blackcnt = 0;
	for (int i = 0; i < 19; i++)
	{
		for (int j = 0; j < 19; j++)
		{
			if (_board[i][j] == WHITE) whitecnt++;
			else if (_board[i][j] == BLACK) blackcnt++;
		}
	}
	std::cout << "white: " << whitecnt << "   black: " << blackcnt << std::endl;		
}

void Gomoku::_reset(){
	_turn = 1;
	_winner = 0;
	_suggPos = Position({-1, -1});
	_moveHistory.clear();
	_moveHistory.push_back(History{-1, -1, -1});
	_board = std::vector<std::vector<uint8_t>>(SIZE, std::vector<uint8_t>(SIZE));
	_captureStone = {0, 0};
	solveTime.clear();
}

void Gomoku::useAI(bool ai){
	_ai = ai;
}

int Gomoku::checkWinning(const Rules::Board& board, int player, const std::array<int, 2>& captures)
{
 return Rules::checkWinning(board, player, captures);
}

bool Gomoku::capture(Rules::Board& board, int x, int y, int player,
                     std::array<int, 2>& captures, std::array<int, 2>* firstCaptured)
{
 return Rules::capture(board, x, y, player, captures, firstCaptured);
}

bool Gomoku::isOnBoard(int x, int y)
{
 return Rules::isOnBoard(x, y);
}

bool Gomoku::checkDoubleFreeThree(const Rules::Board& board, int player, int x, int y)
{
 return Rules::checkDoubleFreeThree(board, player, x, y);
}

Position Gomoku::_gomokuSolver(){
	auto start = std::chrono::high_resolution_clock::now();
	Position pos = _solver.solve(_board, _turn, _captureStone);
	auto end = std::chrono::high_resolution_clock::now();
	double duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

	std::cout << YELLOW << "Execution Time: " << duration / 1e3 << " ms\n" << RESET;
	std::cout << "Solver: completed depth " << _solver.getLastDepth()
		<< ", traversed nodes " << _solver.getLastNodes()
		<< ", state score " << _solver.getLastScore() << std::endl;

	solveTime.push_back(duration / 1e3);

	return pos;
}

std::vector<std::vector<uint8_t>> Gomoku::_updateBorad(int x, int y){
	if (_board[x][y] != 0) return _board;

	if (checkDoubleFreeThree(_board, _turn, x, y))
	{
		std::cout << "Forbidden move: double three is illegal" << std::endl;
		return _board;
	}

	_moveHistory.push_back(History{x, y, _turn});
	_board[x][y] = _turn;
	capture(_board, x, y, _turn, _captureStone);

	_winner = checkWinning(_board, _turn, _captureStone);
	if (_winner != 0) {
		_moveHistory.push_back(History{x, y, _turn});
		return _board;
	}

	_turn = 3 ^ _turn;
	_ui.getWindow().clear(sf::Color(255, 229, 204));
	_ui.draw(_board, _captureStone, _winnerRecord, _ai, _turn, _sugg ? _suggPos : Position{-1, -1}, _winner,
			(_sugg || _moveHistory.empty()) ? History{-1, -1, 0} : _moveHistory.back());
	if (_turn == 2 && _ai) {
		Position pos1 = _gomokuSolver();
		if (pos1.x < 0 || pos1.y < 0 || !isOnBoard(pos1.x, pos1.y)){
			std::cout << "AI found no move." << std::endl;
			_suggPos = Position({-1, -1});
			return _board;
		}
		x = pos1.x;
		y = pos1.y;
		_moveHistory.push_back(History{x, y, _turn});
		_board[x][y] = _turn;
		capture(_board, x, y, _turn, _captureStone);
		_winner = checkWinning(_board, _turn, _captureStone);
		_turn = 1;
	} else if (_sugg){
		_suggPos = _gomokuSolver();
	} else {
		_suggPos.x = -1;
		_suggPos.y = -1;
	}
	return _board;
}

int Gomoku::_winningHandler(){
	if (_winner == 0){ return false; }

	_ui.draw(_board, _captureStone, _winnerRecord, _ai, _turn, _sugg ? _suggPos : Position{-1, -1}, _winner,
			(_sugg || _moveHistory.empty()) ? History{-1, -1, 0} : _moveHistory.back());
	std::string winningMessage = _winner == 1? std::string("White win!") : std::string("Black win!");
	std::cout << winningMessage << std::endl;
	_winnerRecord[_winner - 1] += 1;
	sf::sleep(sf::seconds(3));
	double totalTime = std::reduce(solveTime.begin(), solveTime.end(), 0);
	std::cout << "Average time to find a move: " << totalTime / solveTime.size() << "ms" << std::endl;
	this->_reset();
	return true;
}

int Gomoku::run(){
	float gridSize = _ui.getGridSize();

	while (_ui.getWindow().isOpen()){
		while (const std::optional event = _ui.getWindow().pollEvent()){
			if (event->is<sf::Event::Closed>())
				_ui.getWindow().close();
			if (const auto* mb = event->getIf<sf::Event::MouseButtonPressed>())
			{
				if (mb->button == sf::Mouse::Button::Left){
					sf::Vector2i px = mb->position;

					if (px.x < gridSize || px.y < gridSize) continue;

					unsigned int x = std::round((px.x - gridSize * 1.5) / gridSize);
					unsigned int y = std::round((px.y - gridSize * 1.5) / gridSize);
					std::cout << "Player played at x: " << x << "    y: " << y << std::endl;
					if (x > SIZE - 1 || y > SIZE - 1) continue;
					_board = _updateBorad(x, y);
					/*printBoardCount();
					  _solver._rootPlayer = 3 ^ _turn;
					  _solver._board = _board;
					  _solver._evaluate();*/
					if (_winningHandler()) {}
				}
			}
			if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
				if (keyPressed->scancode == sf::Keyboard::Scan::R){
					std::cout << "Game is reset." << std::endl;
					this->_reset();
				} else if (keyPressed->scancode == sf::Keyboard::Scan::A){
					if (_ai) {_ai = false;}
					else {_ai = true;}
					std::cout << "AI is " << _ai << std::endl;
				} else if (keyPressed->scancode == sf::Keyboard::Scan::S){
					if (_sugg) {
						_sugg = false;
						std::cout << "AI suggestion is activated." << std::endl;
					} else {
						_sugg = true;
						std::cout << "AI suggestion is deactivated." << std::endl;
					}
				}
			}
		}
		_ui.getWindow().clear(sf::Color(255, 229, 204));
		_ui.draw(_board, _captureStone, _winnerRecord, _ai, _turn, _sugg ? _suggPos : Position{-1, -1}, _winner,
				(_sugg || _moveHistory.empty()) ? History{-1, -1, 0} : _moveHistory.back());
	}
	return 0;
}
