#include "Solver.hpp"
#include "Gomoku.hpp"

bool Solver::_onBoard(int x, int y)
{
	return Gomoku::isOnBoard(x, y);
}

void Solver::_load(const std::vector<std::vector<uint8_t>>& board, int turn, const std::array<int, 2>& captureStone)
{
	_board = board;
	_captures = captureStone;
	_rootPlayer = turn == 2 ? 2 : 1;
	_undo.clear();
}

bool Solver::_play(const int x, const int y, int player)
{
	_board[x][y] = static_cast<uint8_t>(player);
	if (Gomoku::checkDoubleFreeThree(_board, player, x, y)) {
		_board[x][y] = 0;
		return false;
	}

	Undo undo;
	undo.x = x;
	undo.y = y;
	undo.capturesBefore = _captures;
	undo.captured = false;

	std::array<int, 2> firstCaptured;
	if (Gomoku::capture(_board, x, y, player, _captures, &firstCaptured)) {
		undo.capturex1 = firstCaptured[0];
		undo.capturey1 = firstCaptured[1];
		undo.capturex2 = firstCaptured[0] + firstCaptured[0] - x;
		undo.capturey2 = firstCaptured[1] + firstCaptured[1] - y;
		undo.captured = true;
	}

	_undo.push_back(undo);
	return true;
}

void Solver::_unplay()
{
	const Undo undo = _undo.back();
	_undo.pop_back();

	const int player = _board[undo.x][undo.y];
	_board[undo.x][undo.y] = 0;
	if (undo.captured)
	{
		_board[undo.capturex1][undo.capturey1] = static_cast<uint8_t>(3 ^ player);
		_board[undo.capturex2][undo.capturey2] = static_cast<uint8_t>(3 ^ player);
	}
	_captures = undo.capturesBefore;
}

// If player plays a stone in pos (x, y), evaluate the score for this position
int Solver::_scorePosition(int x, int y, int player) const
{
	const int opponent = 3 ^ player;

	static const int dirs[4][2] = {{-1, 1}, {0, 1}, {1, 1}, {1, 0}};
	int maxScore = STONE;
	for (const auto& d : dirs)
	{
		bool lastEmpty = false;
		bool hardStop = false;
		int curScore = 0, stoneCount = 1, emptyCount = 0;
		int dx = d[0], dy = d[1];
		for (int step = -1; step >= -4; --step) {
			const int nx = x + dx * step;
			const int ny = y + dy * step;

			if (!_onBoard(nx, ny) || _board[nx][ny] == opponent)
			{
				if (!lastEmpty) hardStop = true;
				break;
			}

			const uint8_t cell = _board[nx][ny];
			if (cell == player)
			{
				++stoneCount;
				lastEmpty = false;
			}
			else if (cell == 0)
			{
				++emptyCount;
				if (lastEmpty) break;
				lastEmpty = true;
			}
		}
		lastEmpty = false;
		for (int step = 1; step <= 4; ++step) {
			const int nx = x + dx * step;
			const int ny = y + dy * step;

			if (!_onBoard(nx, ny)) break;
			{
				if (!lastEmpty) hardStop = true;
				break;
			}

			const uint8_t cell = _board[nx][ny];
			if (cell == player)
			{
				++stoneCount;
				lastEmpty = false;
			}
			else if (cell == 0)
			{
				++emptyCount;
				if (lastEmpty) break;
				lastEmpty = true;
			}
		}

		bool longLine = stoneCount + emptyCount >= 5;

		if (!longLine)
			curScore = STONE;
		else if (stoneCount >= 5 && emptyCount <= 1)
			curScore = FIVE;
		else if (stoneCount >= 4)
			curScore = emptyCount ? OPEN_FOUR : FOUR;
		else if (stoneCount >= 3)
			curScore = emptyCount >= 2 ? OPEN_THREE : THREE;
		else if (stoneCount >= 2)
			curScore = emptyCount >= 3 ? OPEN_TWO : TWO;
		if (hardStop) curScore /= 2;
		maxScore = std::max(maxScore, curScore);
	}
	return maxScore;
}

int Solver::_captureScore(int player) const
{
	int score = _captures[player - 1] * CAPTURE_PROGRESS;
	const int opponent = 3 ^ player;

	for (int x = 0; x < static_cast<int>(SIZE); ++x) {
		for (int y = 0; y < static_cast<int>(SIZE); ++y) {
			if (_board[x][y] != player)
				continue;

			static const int dx[4] = {1, 0, 1, 1};
			static const int dy[4] = {0, 1, 1, -1};
			for (int d = 0; d < 4; ++d) {
				for (int sign : {-1, 1}) {
					int x1 = x + dx[d] * sign;
					int y1 = y + dy[d] * sign;
					int x2 = x + 2 * dx[d] * sign;
					int y2 = y + 2 * dy[d] * sign;
					int x3 = x + 3 * dx[d] * sign;
					int y3 = y + 3 * dy[d] * sign;

					if (!_onBoard(x1, y1) || !_onBoard(x2, y2) || !_onBoard(x3, y3))
						continue;
					if (_board[x1][y1] == opponent && _board[x2][y2] == opponent && _board[x3][y3] == 0)
						score -= CAPTURE_PROGRESS;
				}
			}
		}
	}
	return score;
}

int Solver::_evaluate() const
{
	int mine = -INF, theirs = -INF;
	const int opponent = 3 ^ _rootPlayer;
	for (int x = 0; x < static_cast<int>(SIZE); x++)
	{
		for (int y = 0; y < static_cast<int>(SIZE); y++)
		{
			if (_board[x][y] == _rootPlayer)
				mine = std::max(mine, _scorePosition(x, y, _rootPlayer));
			else if (_board[x][y] == opponent)
				theirs = std::max(theirs, _scorePosition(x, y, opponent));
		}
	}

	mine += _captureScore(_rootPlayer);
	theirs += _captureScore(3 ^ _rootPlayer);
	return mine - theirs;
}

uint64_t Solver::_hash(int player) const
{
	uint64_t hash = 1469598103934665603ULL;
	for (const auto& row : _board) {
		for (uint8_t cell : row) {
			hash ^= cell;
			hash *= 1099511628211ULL;
		}
	}
	hash ^= static_cast<uint64_t>(_captures[0] + 31 * _captures[1] + 961 * player);
	hash *= 1099511628211ULL;
	return hash;
}

int Solver::_moveScore(int x, int y, int player)
{
	if (!_play(x, y, player)) return -INF;

	int score = 0;
	if (Gomoku::checkWinning(_board, player, _captures)) score += WIN / 2;

	score += _scorePosition(x, y, player);
	std::cout << MAGENTA << "x: " << x << "     y: " << y << std::endl;
	std::cout << "score: " << score << std::endl;
	score += _scorePosition(x, y, 3 ^ player) * 6 / 5;
	std::cout << "score: " << score << std::endl;

	score += (_captures[player - 1] * CAPTURE_PROGRESS);
	std::cout << "score: " << score << std::endl;
	const int center = static_cast<int>(SIZE / 2);
	score -= std::abs(x - center) + std::abs(y - center);
	std::cout << "score: " << score << RESET << std::endl;
	_unplay();
	return score;
}

std::vector<Solver::Move> Solver::_generateMoves(int player)
{
	std::vector<Move> moves;
	moves.reserve(CELLS);

	bool hasStone = false;
	for (const auto& row : _board)
	{
		for (uint8_t cell : row)
		{
			if (cell)
			{
				hasStone = true;
				break;
			}
		}
		if (hasStone) break;
	}

	if (!hasStone) return {{SIZE / 2, SIZE / 2, 0}};

	bool near[CELLS]{};
	for (int x = 0; x < static_cast<int>(SIZE); ++x) {
		for (int y = 0; y < static_cast<int>(SIZE); ++y) {
			if (_board[x][y] == 0) continue;

			for (int dx = -2; dx <= 2; ++dx) {
				for (int dy = -2; dy <= 2; ++dy) {
					const int nx = x + dx;
					const int ny = y + dy;
					if (_onBoard(nx, ny) && _board[nx][ny] == 0)
						near[nx * SIZE + ny] = true;
				}
			}
		}
	}

	for (int x = 0; x < static_cast<int>(SIZE); ++x) {
		for (int y = 0; y < static_cast<int>(SIZE); ++y) {
			if (!near[x * SIZE + y]) continue;
			moves.push_back({x, y, _moveScore(x, y, player)});
		}
	}

	std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) { return a.score > b.score; });
	if (moves.size() > MAX_MOVES) moves.resize(MAX_MOVES);

	return moves;
}

bool Solver::_timeUp()
{
	if (_aborted) return true;

	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - _start).count();
	if (elapsed >= TIME_LIMIT_MS) _aborted = true;
	return _aborted;
}
/*
int Solver::_minmax(unsigned int depth, int alpha, int beta, int player)
{
	if (depth >= MAX_DEPTH) return _evaluate();

	const bool maximizing = (player == _rootPlayer);
	const int opponent = 3 ^ player;
	int best = maximizing ? -INF : INF;

	std::vector<Move> moves = _generateMoves(player);

	for (const Move& move : moves)
	{
		if (!_play(move.x, move.y, player))
			continue;

		int score = _minmax(depth + 1, alpha, beta, opponent);
		_unplay();

		if (maximizing)
		{
			best = std::max(best, score);
			alpha = std::max(alpha, best);
		}
		else
		{
			best = std::min(best, score);
			beta = std::min(beta, best);
		}

		if (alpha >= beta)
			break;
	}

	return best;
}
*/
int Solver::_minmax(unsigned int depth, int alpha, int beta, int player)
{
	_lastDepth = std::max(_lastDepth, depth);
	if (((++_nodes & 1023ULL) == 0 && _timeUp()) || depth >= MAX_DEPTH) return _evaluate();

	const int opponent = 3 ^ player, originalAlpha = alpha, originalBeta = beta;
	int tableMove = -1;
	const uint64_t key = _hash(player);

	auto cached = _table.find(key);
	if (cached != _table.end()) {
		tableMove = cached->second.move;
		if (cached->second.bound == 0) return cached->second.score;
		if (cached->second.bound == 1) alpha = std::max(alpha, cached->second.score);
		else beta = std::min(beta, cached->second.score);
		if (alpha >= beta) return cached->second.score;
	}

	std::vector<Move> moves = _generateMoves(player);
	for (const auto& m : moves)
		std::cout << GREEN << "Move: \nX: " << m.x << "   Y: " << m.y << "    score: " << m.score << RESET << std::endl;
	if (moves.empty()) return _evaluate();

	const bool maximizing = player == _rootPlayer;
	int best = maximizing ? -INF : INF;
	int bestX = -1, bestY = -1;

	if (tableMove >= 0) {
		auto it = std::find_if(moves.begin(), moves.end(),
				[tableMove](const Move& move) { return move.x == static_cast<int>(tableMove / SIZE) && move.y == static_cast<int>(tableMove % SIZE); });
		if (it != moves.end()) std::rotate(moves.begin(), it, it + 1);
	}

	for (const Move& move : moves) {
		if (!_play(move.x, move.y, player)) continue;

		const int value = _minmax(depth + 1, alpha, beta, opponent);
		_unplay();

		if (_aborted) return 0;

		if (maximizing) {
			if (value > best) {
				best = value;
				bestX = move.x;
				bestY = move.y;
			}
			alpha = std::max(alpha, best);
		} else {
			if (value < best) {
				best = value;
				bestX = move.x;
				bestY = move.y;
			}
			beta = std::min(beta, best);
		}

		if (alpha >= beta) break;
	}

	if (bestX < 0) return _evaluate();
	_pv[_undo.size()] = bestX * SIZE + bestY;
	if (!_aborted) {
		TableEntry entry{best, static_cast<int>(bestX * SIZE + bestY), depth, 0};
		if (best <= originalAlpha) entry.bound = 2;
		else if (best >= originalBeta) entry.bound = 1;
		if (_table.size() > 250000) _table.clear();
		_table[key] = entry;
	}
	return best;
}

void Solver::_buildPV()
{
	_lastPV.clear();
	for (unsigned int i = 0; i <= MAX_DEPTH && _pv[i] >= 0; ++i) {
		if (i != 0)
			_lastPV += ' ';
		_lastPV += std::to_string(_pv[i] / SIZE);
		_lastPV += ',';
		_lastPV += std::to_string(_pv[i] % SIZE);
	}
}

Position Solver::solve(const std::vector<std::vector<uint8_t>>& board, int turn, const std::array<int, 2>& captureStone)
{
	_load(board, turn, captureStone);
	_table.clear();
	_nodes = 0;
	_aborted = false;
	_lastDepth = 0;
	_lastNodes = 0;
	_lastScore = 0;
	_lastPV.clear();
	std::fill(std::begin(_pv), std::end(_pv), -1);
	_start = std::chrono::steady_clock::now();

	bool hasStone = false;
	for (const auto& row : _board)
	{
		for (uint8_t cell : row)
		{
			if (cell)
			{
				hasStone = true;
				break;
			}
		}
		if (hasStone) break;
	}
	if (!hasStone) {
		_lastNodes = 1;
		_lastScore = 0;
		return Position({static_cast<int>(SIZE / 2), static_cast<int>(SIZE / 2)});
	}

	int bestX = -1, bestY = -1;
	int bestScore = -INF;

	std::fill(std::begin(_pv), std::end(_pv), -1);

	const int score = _minmax(0, -INF, INF, _rootPlayer);

	if (_pv[0] >= 0) {
		bestX = _pv[0] / SIZE;
		bestY = _pv[0] % SIZE;
		bestScore = score;
		_lastDepth = 10;
		_lastScore = score;
	}

	if (bestX < 0) {
		const std::vector<Move> fallback = _generateMoves(_rootPlayer);
		if (!fallback.empty()) {
			bestX = fallback.front().x;
			bestY = fallback.front().y;
			bestScore = fallback.front().score;
		}
	}

	_lastNodes = _nodes;
	_lastScore = bestScore;
	_buildPV();

	return Position({bestX, bestY});
}

unsigned int Solver::getLastDepth() const
{
	return _lastDepth;
}

unsigned long long Solver::getLastNodes() const
{
	return _lastNodes;
}

int Solver::getLastScore() const
{
	return _lastScore;
}

std::string Solver::getLastPV() const
{
	return _lastPV;
}
