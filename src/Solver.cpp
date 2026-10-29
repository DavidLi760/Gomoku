#include "Solver.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
constexpr int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
constexpr int power3[8] = {1, 3, 9, 27, 81, 243, 729, 2187};

uint64_t mix(uint64_t x)
{
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

struct Pattern {
    int score = 0;
    int fours = 0;
    bool three = false;
    bool five = false;
};

// Normalize each direction to empty/own/blocked. The center is the move.
// All 3^8 neighborhoods are evaluated once, instead of rescanned at every node.
const std::array<Pattern, 6561>& patterns()
{
    static const auto table = [] {
        std::array<Pattern, 6561> result{};
        for (int code = 0; code < 6561; ++code) {
            int cells[9]{};
            cells[4] = 1;
            int value = code;
            for (int k = 0; k < 9; ++k) {
                if (k == 4) continue;
                cells[k] = value % 3;
                value /= 3;
            }
            auto& p = result[code];
            unsigned int winningPoints = 0;
            for (int start = 0; start <= 4; ++start) {
                int stones = 0, empty = 0, hole = -1;
                for (int k = start; k < start + 5; ++k) {
                    stones += cells[k] == 1;
                    if (cells[k] == 0) { ++empty; hole = k; }
                }
                if (stones == 5) p.five = true;
                if (stones == 4 && empty == 1) winningPoints |= 1u << hole;
                if (stones == 2 && empty == 3) p.score += 12;
                if (stones == 3 && empty == 2) p.score += 150;
            }
            for (int start = 1; start <= 4; ++start) {
                if (cells[start - 1] || cells[start + 4]) continue;
                int stones = 0, empty = 0;
                for (int k = start; k < start + 4; ++k) {
                    stones += cells[k] == 1;
                    empty += cells[k] == 0;
                }
                if (stones == 3 && empty == 1) p.three = true;
            }
            while (winningPoints) {
                winningPoints &= winningPoints - 1;
                ++p.fours;
            }
            p.score += p.five ? 1000000 : p.fours >= 2 ? 120000 :
                       p.fours ? 18000 : p.three ? 4000 : 0;
        }
        return result;
    }();
    return table;
}
}

Solver::Solver() : _table(TABLE_SIZE)
{
    (void)patterns();
    // Each cell belongs to exactly four lines, including short edge diagonals.
    for (int d = 0; d < 4; ++d) {
        for (int x = 0; x < Rules::SIZE; ++x) {
            for (int y = 0; y < Rules::SIZE; ++y) {
                if (Rules::isOnBoard(x - directions[d][0], y - directions[d][1])) continue;
                const int id = static_cast<int>(_lines.size());
                Line line;
                for (int nx = x, ny = y; Rules::isOnBoard(nx, ny);
                     nx += directions[d][0], ny += directions[d][1]) {
                    const int index = nx * SIZE + ny;
                    line.cells.push_back(index);
                    _cellLines[index][d] = id;
                }
                _lines.push_back(std::move(line));
            }
        }
    }
    _undo.reserve(MAX_PLY);
}

void Solver::setTimeLimit(int milliseconds) { _timeLimitMs = std::max(1, milliseconds); }
void Solver::setNodeLimit(uint64_t nodes) { _nodeLimit = nodes; }

void Solver::_load(const Rules::Board& board, int turn, const std::array<int, 2>& captures)
{
    if (board.size() != SIZE || (turn != BLACK && turn != WHITE) ||
        captures[0] < 0 || captures[1] < 0)
        throw std::invalid_argument("Invalid Gomoku position");
    for (const auto& row : board) {
        if (row.size() != SIZE) throw std::invalid_argument("Invalid board size");
        for (auto cell : row)
            if (cell > WHITE) throw std::invalid_argument("Invalid board cell");
    }
    _board.assign(SIZE, std::vector<uint8_t>(SIZE));
    _rootPlayer = turn;
    _captures = captures;
    _boardHash = 0;
    _stoneCount = 0;
    _neighbors.fill(0);
    _undo.clear();
    for (auto& dirty : _featureDirty) dirty.fill(true);
    for (int x = 0; x < Rules::SIZE; ++x)
        for (int y = 0; y < Rules::SIZE; ++y)
            if (board[x][y]) _setCell(x * SIZE + y, board[x][y]);
    _positionScore = {0, 0};
    _fives = {0, 0};
    for (auto& line : _lines) { line.scores = {0, 0}; line.fives = {0, 0}; }
    for (int line = 0; line < static_cast<int>(_lines.size()); ++line) _scoreLine(line);
}

void Solver::_setCell(int index, int value)
{
    const int x = index / SIZE, y = index % SIZE;
    const int previous = _board[x][y];
    if (previous) _boardHash ^= mix(index * 3 + previous);
    if (value) _boardHash ^= mix(index * 3 + value);
    _board[x][y] = static_cast<uint8_t>(value);
    const int delta = (value != EMPTY) - (previous != EMPTY);
    _stoneCount += delta;
    for (int dx = -2; dx <= 2; ++dx)
        for (int dy = -2; dy <= 2; ++dy)
            if (Rules::isOnBoard(x + dx, y + dy))
                _neighbors[(x + dx) * SIZE + y + dy] += delta;
    for (const auto& d : directions) {
        for (int step = -4; step <= 4; ++step) {
            int nx = x + step * d[0], ny = y + step * d[1];
            if (Rules::isOnBoard(nx, ny)) {
                _featureDirty[0][nx * SIZE + ny] = true;
                _featureDirty[1][nx * SIZE + ny] = true;
            }
        }
    }
}

void Solver::_scoreLine(int id)
{
    auto& line = _lines[id];
    const int length = static_cast<int>(line.cells.size());
    for (int player : {BLACK, WHITE}) {
        const int color = player - 1;
        _positionScore[color] -= line.scores[color];
        _fives[color] -= line.fives[color];
        int score = 0, fives = 0;
        const auto cell = [&](int k) -> int {
            if (k < 0 || k >= length) return 3 ^ player;
            const int index = line.cells[k];
            return _board[index / SIZE][index % SIZE];
        };
        for (int start = 0; start < length;) {
            if (cell(start) != player) { ++start; continue; }
            int end = start + 1;
            while (end < length && cell(end) == player) ++end;
            const int count = end - start;
            const int open = (cell(start - 1) == EMPTY) + (cell(end) == EMPTY);
            if (count >= 5) { score += 800000; ++fives; }
            else if (count == 4) score += open == 2 ? 70000 : open ? 12000 : 0;
            else if (count == 3) score += open == 2 ? 3000 : open ? 250 : 0;
            else if (count == 2) score += open == 2 ? 180 : open ? 20 : 0;
            start = end;
        }
        // Windows add the broken threats missed by contiguous-run scoring.
        for (int start = 0; start + 5 <= length; ++start) {
            int stones = 0, empty = 0, hole = -1;
            for (int k = start; k < start + 5; ++k) {
                stones += cell(k) == player;
                if (cell(k) == EMPTY) { ++empty; hole = k; }
            }
            if (stones == 4 && empty == 1 && hole > start && hole < start + 4) score += 12000;
            else if (stones == 3 && empty == 2) score += 80;
            else if (stones == 2 && empty == 3) score += 8;
        }
        // A vulnerable pair is valuable to the opponent even without a line threat.
        for (int k = 0; k + 4 <= length; ++k) {
            if (cell(k + 1) == player && cell(k + 2) == player &&
                ((cell(k) == (3 ^ player) && cell(k + 3) == EMPTY) ||
                 (cell(k) == EMPTY && cell(k + 3) == (3 ^ player)))) score -= 400;
        }
        line.scores[color] = score;
        line.fives[color] = fives;
        _positionScore[color] += score;
        _fives[color] += fives;
    }
}

void Solver::_refreshLines(const Undo& undo)
{
    std::array<bool, 112> dirty{};
    for (int d = 0; d < 4; ++d) dirty[_cellLines[undo.index][d]] = true;
    for (int k = 0; k < undo.count; ++k)
        for (int d = 0; d < 4; ++d) dirty[_cellLines[undo.captured[k]][d]] = true;
    for (int id = 0; id < static_cast<int>(_lines.size()); ++id)
        if (dirty[id]) _scoreLine(id);
}

Solver::Features Solver::_features(int index, int player) const
{
    auto& cached = _featureCache[player - 1][index];
    auto& dirty = _featureDirty[player - 1][index];
    if (!dirty) return cached;
    Features f;
    const int x = index / SIZE, y = index % SIZE;
    if (_board[x][y] != EMPTY) { dirty = false; cached = f; return f; }
    const auto& table = patterns();
    for (const auto& d : directions) {
        int code = 0, digit = 0;
        for (int step = -4; step <= 4; ++step) {
            if (step == 0) continue;
            const int nx = x + step * d[0], ny = y + step * d[1];
            int cell = 2;
            if (Rules::isOnBoard(nx, ny))
                cell = _board[nx][ny] == EMPTY ? 0 : _board[nx][ny] == player ? 1 : 2;
            code += cell * power3[digit++];
        }
        const auto& p = table[code];
        f.score += p.score;
        f.fours += p.fours;
        f.threes += p.three;
        f.five |= p.five;
        for (int sign : {-1, 1}) {
            int nx = x + 3 * sign * d[0], ny = y + 3 * sign * d[1];
            if (Rules::isOnBoard(nx, ny) &&
                _board[x + sign * d[0]][y + sign * d[1]] == (3 ^ player) &&
                _board[x + 2 * sign * d[0]][y + 2 * sign * d[1]] == (3 ^ player)) {
                if (_board[nx][ny] == player) f.captured += 2;
                if (_board[nx][ny] == EMPTY) f.captureSetup = true;
            }
        }
    }
    f.legal = f.threes < 2;
    if (f.fours >= 2) f.score += 120000;
    if (f.fours && f.threes) f.score += 30000;
    cached = f;
    dirty = false;
    return f;
}

bool Solver::_play(int index, int player)
{
    if (index < 0 || index >= static_cast<int>(CELLS) || !_features(index, player).legal) return false;
    Undo undo;
    undo.index = index;
    undo.player = player;
    undo.capturesBefore = _captures;
    const int x = index / SIZE, y = index % SIZE;
    _setCell(index, player);
    for (const auto& d : directions) {
        for (int sign : {-1, 1}) {
            int nx = x + 3 * sign * d[0], ny = y + 3 * sign * d[1];
            if (!Rules::isOnBoard(nx, ny) || _board[nx][ny] != player) continue;
            int first = (x + sign * d[0]) * SIZE + y + sign * d[1];
            int second = (x + 2 * sign * d[0]) * SIZE + y + 2 * sign * d[1];
            if (_board[first / SIZE][first % SIZE] != (3 ^ player) ||
                _board[second / SIZE][second % SIZE] != (3 ^ player)) continue;
            undo.captured[undo.count++] = first;
            undo.captured[undo.count++] = second;
            _setCell(first, EMPTY);
            _setCell(second, EMPTY);
        }
    }
    _captures[player - 1] += undo.count;
    _refreshLines(undo);
    _undo.push_back(undo);
    return true;
}

void Solver::_unplay()
{
    const Undo undo = _undo.back();
    _undo.pop_back();
    _setCell(undo.index, EMPTY);
    for (int k = 0; k < undo.count; ++k) _setCell(undo.captured[k], 3 ^ undo.player);
    _captures = undo.capturesBefore;
    _refreshLines(undo);
}

int Solver::_winner(int lastPlayer) const
{
    if (_captures[0] >= CAPTURE_TO_WIN) return BLACK;
    if (_captures[1] >= CAPTURE_TO_WIN) return WHITE;
    if (!_fives[0] && !_fives[1]) return EMPTY;
    return Rules::checkWinning(_board, lastPlayer, _captures);
}

bool Solver::_wins(int index, int player, const Features& f)
{
    if (!f.legal || (!f.five && _captures[player - 1] + f.captured < CAPTURE_TO_WIN)) return false;
    if (!_play(index, player)) return false;
    const bool result = _winner(player) == player;
    _unplay();
    return result;
}

bool Solver::_hasImmediateWin(int player)
{
    for (int i = 0; i < static_cast<int>(CELLS); ++i) {
        if ((i & 31) == 0 && _timeUp()) return false;
        if (_board[i / SIZE][i % SIZE] || !_neighbors[i]) continue;
        if (_wins(i, player, _features(i, player))) return true;
    }
    return false;
}

int Solver::_staticEval(int player) const
{
    int score = _positionScore[player - 1] - _positionScore[(3 ^ player) - 1];
    const auto progress = [](int captured) { return 150 * captured * captured + 200 * captured; };
    score += progress(_captures[player - 1]) - progress(_captures[(3 ^ player) - 1]);
    // A static feature must never masquerade as a proven terminal result.
    return std::clamp(score, -WIN / 4, WIN / 4);
}

int Solver::_evaluate() const
{
    if (_captures[_rootPlayer - 1] >= CAPTURE_TO_WIN) return WIN;
    if (_captures[(3 ^ _rootPlayer) - 1] >= CAPTURE_TO_WIN) return -WIN;
    return _staticEval(_rootPlayer);
}

uint64_t Solver::_hash(int player) const
{
    return _boardHash ^ mix(0x10000ULL + player) ^
           mix(0x20000ULL + _captures[0]) ^ mix(0x30000ULL + _captures[1]);
}

Solver::MoveList Solver::_generateMoves(int player, int ply)
{
    MoveList result;
    if (_stoneCount == 0) { result.moves.push_back({static_cast<int>(CELLS / 2), 0, false}); return result; }
    const int opponent = 3 ^ player;
    std::vector<Move> wins;
    std::vector<int> opponentWins;
    bool threatened = _fives[opponent - 1] != 0;
    for (int index = 0; index < static_cast<int>(CELLS); ++index) {
        if ((index & 15) == 0 && _timeUp()) return result;
        if (_board[index / SIZE][index % SIZE] || !_neighbors[index]) continue;
        const Features attack = _features(index, player);
        const Features defense = _features(index, opponent);
        if (_wins(index, opponent, defense)) { threatened = true; opponentWins.push_back(index); }
        if (!attack.legal) continue;
        if (_wins(index, player, attack)) wins.push_back({index, WIN, true});
        const bool forcing = attack.fours || attack.five ||
                             (attack.captured && _captures[player - 1] + attack.captured >= 8);
        int score = 2 * attack.score + (defense.legal ? defense.score : 0);
        score += attack.captured * (2000 + 1000 * _captures[player - 1]);
        if (defense.legal) score += defense.captured * (1000 + 800 * _captures[opponent - 1]);
        score += std::min(15000, _history[player - 1][index]);
        if (ply < MAX_PLY && (_killers[ply][0] == index || _killers[ply][1] == index)) score += 16000;
        score -= std::abs(index / static_cast<int>(SIZE) - 9) + std::abs(index % static_cast<int>(SIZE) - 9);
        if (forcing) score += 1000000;
        else if (attack.captured) score += 500000;
        result.moves.push_back({index, score, forcing, attack.captured != 0});
    }
    if (!wins.empty()) {
        result.moves = std::move(wins);
        result.won = true;
        return result;
    }
    if (threatened) {
        // A direct block is not the only defense: a capture can remove several
        // threats. Verify all legal candidates before applying any width limit.
        result.forced = true;
        std::vector<Move> defenses;
        for (auto move : result.moves) {
            if (_timeUp()) return result;
            const auto f = _features(move.index, player);
            // A quiet defense must occupy the winning point or set up a capture
            // that makes the future five breakable. Existing fives need a capture now.
            if (!f.captured && (_fives[opponent - 1] ||
                (!f.captureSetup && (opponentWins.size() != 1 || opponentWins[0] != move.index)))) continue;
            if (!_play(move.index, player)) continue;
            const int winner = _winner(player);
            const bool safe = winner == player || (winner == EMPTY && !_hasImmediateWin(opponent));
            _unplay();
            if (_aborted) return result;
            if (safe) defenses.push_back(move);
        }
        result.moves = std::move(defenses);
        result.lost = result.moves.empty();
    }
    std::sort(result.moves.begin(), result.moves.end(), [](const Move& a, const Move& b) {
        return a.score != b.score ? a.score > b.score : a.index < b.index;
    });
    return result;
}

bool Solver::_timeUp()
{
    if (_aborted) return true;
    if (_nodeLimit && _nodes >= _nodeLimit) _aborted = true;
    if (std::chrono::steady_clock::now() - _start >= std::chrono::milliseconds(_timeLimitMs)) _aborted = true;
    return _aborted;
}

int Solver::_search(int depth, int ply, int alpha, int beta, int player, int qleft)
{
    ++_nodes;
    if (_timeUp()) return 0;
    const int winner = _winner(3 ^ player);
    if (winner) return winner == player ? WIN - ply : -WIN + ply;
    if (ply >= MAX_PLY - 1) return _staticEval(player);
    const bool quiescence = depth <= 0;
    if (quiescence && qleft <= 0) return _staticEval(player);
    const int originalAlpha = alpha;
    const uint64_t key = _hash(player);
    auto& entry = _table[key & (TABLE_SIZE - 1)];
    int tableMove = -1;
    if (!quiescence && entry.key == key && entry.depth >= 0) {
        tableMove = entry.move;
        if (entry.depth >= depth) {
            int score = entry.score;
            if (score >= MATE_THRESHOLD) score -= ply;
            if (score <= -MATE_THRESHOLD) score += ply;
            if (entry.bound == EXACT || (entry.bound == LOWER && score >= beta) ||
                (entry.bound == UPPER && score <= alpha)) {
                if (ply == 0) _iterationMove = tableMove;
                return score;
            }
        }
    }
    MoveList list = _generateMoves(player, ply);
    if (_aborted) return 0;
    if (list.won) {
        if (ply == 0) _iterationMove = list.moves.front().index;
        return WIN - ply - 1;
    }
    if (list.lost) return -WIN + ply + 2;
    if (list.moves.empty()) return 0;
    if (tableMove >= 0) {
        auto it = std::find_if(list.moves.begin(), list.moves.end(),
                               [tableMove](const Move& m) { return m.index == tableMove; });
        if (it != list.moves.end()) std::rotate(list.moves.begin(), it, it + 1);
    }
    int best = -INF;
    int bestMove = -1;
    if (quiescence && !list.forced) {
        best = _staticEval(player);
        if (best >= beta) return best;
        alpha = std::max(alpha, best);
    }
    int searched = 0;
    const int width = ply == 0 ? 36 : depth >= 3 ? 20 : 28;
    for (const auto& move : list.moves) {
        if (quiescence && !list.forced && !move.forcing) continue;
        if (!quiescence && !list.forced && searched >= width && !move.forcing && !move.capture) continue;
        if (!_play(move.index, player)) continue;
        const int nextDepth = depth - 1;
        int score;
        if (searched == 0) {
            score = -_search(nextDepth, ply + 1, -beta, -alpha, 3 ^ player, qleft - quiescence);
        } else {
            // Reduce late quiet moves, then verify at full depth if they improve alpha.
            const int reduction = !quiescence && !list.forced && !move.forcing && !move.capture &&
                                  depth >= 3 && searched >= 5 ? 1 : 0;
            score = -_search(nextDepth - reduction, ply + 1, -alpha - 1, -alpha,
                             3 ^ player, qleft - quiescence);
            if (!_aborted && reduction && score > alpha)
                score = -_search(nextDepth, ply + 1, -alpha - 1, -alpha, 3 ^ player, qleft);
            if (!_aborted && score > alpha && score < beta)
                score = -_search(nextDepth, ply + 1, -beta, -alpha, 3 ^ player, qleft - quiescence);
        }
        _unplay();
        if (_aborted) return 0;
        ++searched;
        if (score > best) { best = score; bestMove = move.index; }
        if (score > alpha) alpha = score;
        if (ply == 0 && bestMove >= 0) _iterationMove = bestMove;
        if (alpha >= beta) {
            if (!quiescence && !move.forcing && !move.capture) {
                auto& history = _history[player - 1][move.index];
                history = std::min(30000, history + depth * depth * 32);
                if (_killers[ply][0] != move.index) {
                    _killers[ply][1] = _killers[ply][0];
                    _killers[ply][0] = move.index;
                }
            }
            break;
        }
    }
    if (!quiescence && bestMove >= 0) {
        int saved = best;
        if (saved >= MATE_THRESHOLD) saved += ply;
        if (saved <= -MATE_THRESHOLD) saved -= ply;
        // Reacquire after recursion; a child can replace this same slot.
        auto& target = _table[key & (TABLE_SIZE - 1)];
        if (target.key == key || target.depth <= depth) {
            target = {key, saved, static_cast<int16_t>(bestMove), static_cast<int8_t>(depth),
                      best <= originalAlpha ? UPPER : best >= beta ? LOWER : EXACT};
        }
    }
    return best;
}

void Solver::_buildPV(int player, unsigned int depth)
{
    _lastPV.clear();
    int made = 0;
    int index = _iterationMove;
    for (unsigned int ply = 0; ply < depth && index >= 0; ++ply) {
        if (!_play(index, player)) break;
        ++made;
        if (!_lastPV.empty()) _lastPV += ' ';
        _lastPV += std::to_string(index / SIZE) + ',' + std::to_string(index % SIZE);
        if (_winner(player)) break;
        player = 3 ^ player;
        const uint64_t key = _hash(player);
        const auto& entry = _table[key & (TABLE_SIZE - 1)];
        index = entry.key == key ? entry.move : -1;
    }
    while (made-- > 0) _unplay();
}

Position Solver::solve(const Rules::Board& board, int turn, const std::array<int, 2>& captures)
{
    _start = std::chrono::steady_clock::now();
    _aborted = false;
    _nodes = 0;
    _lastDepth = 0;
    _lastNodes = 0;
    _lastScore = 0;
    _lastPV.clear();
    _iterationMove = -1;
    _load(board, turn, captures);
    std::fill(_table.begin(), _table.end(), TableEntry{});
    for (auto& history : _history) history.fill(0);
    for (auto& killers : _killers) killers.fill(-1);
    if (_winner(3 ^ turn)) return {-1, -1};
    if (!_stoneCount) { _lastNodes = 1; _lastPV = "9,9"; return {9, 9}; }
    // A legal, deterministic fallback exists even if the deadline interrupts
    // initial move ordering. Never continue searching after the budget expires.
    int bestMove = -1, nearest = INF;
    for (int i = 0; i < static_cast<int>(CELLS); ++i) {
        if (!_features(i, turn).legal) continue;
        int distance = std::abs(i / static_cast<int>(SIZE) - 9) + std::abs(i % static_cast<int>(SIZE) - 9);
        if (distance < nearest) { nearest = distance; bestMove = i; }
    }
    if (bestMove < 0) return {-1, -1};
    int bestScore = _staticEval(turn);
    MoveList initial = _generateMoves(turn, 0);
    if (!_aborted && !initial.moves.empty()) bestMove = initial.moves.front().index;
    if (!_aborted && initial.won) {
        _iterationMove = bestMove;
        _lastScore = WIN - 1;
        _lastDepth = 1;
        _lastNodes = _nodes;
        _buildPV(turn, 1);
        return {bestMove / static_cast<int>(SIZE), bestMove % static_cast<int>(SIZE)};
    }
    for (unsigned int depth = 1; depth <= MAX_DEPTH && !_timeUp(); ++depth) {
        _iterationMove = -1;
        const int score = _search(depth, 0, -INF, INF, turn, QUIESCENCE_DEPTH);
        if (_aborted) break;
        if (_iterationMove >= 0) {
            bestMove = _iterationMove;
            bestScore = score;
            _lastDepth = depth;
            _buildPV(turn, depth);
        }
        if (std::abs(score) >= MATE_THRESHOLD || initial.lost) break;
    }
    _lastNodes = _nodes;
    _lastScore = bestScore;
    if (_lastPV.empty()) _lastPV = std::to_string(bestMove / SIZE) + ',' + std::to_string(bestMove % SIZE);
    return {bestMove / static_cast<int>(SIZE), bestMove % static_cast<int>(SIZE)};
}

unsigned int Solver::getLastDepth() const { return _lastDepth; }
unsigned long long Solver::getLastNodes() const { return _lastNodes; }
int Solver::getLastScore() const { return _lastScore; }
std::string Solver::getLastPV() const { return _lastPV; }
