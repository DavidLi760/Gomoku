#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include "Position.hpp"
#include "Rules.hpp"

class Solver {
public:
    enum Cell { EMPTY = 0, BLACK = 1, WHITE = 2 };
    static constexpr unsigned int SIZE = Rules::SIZE;
    static constexpr unsigned int CELLS = SIZE * SIZE;
    static constexpr unsigned int MIN_DEPTH = 1;
    static constexpr unsigned int MAX_DEPTH = 24;
    static constexpr int CAPTURE_TO_WIN = Rules::CAPTURE_TO_WIN;
    static constexpr int WIN = 100000000;
    static constexpr int INF = 2000000000;

    Solver();
    Position solve(const Rules::Board& board, int turn, const std::array<int, 2>& captures);
    // Set before solve(). The default gives the stronger search two seconds.
    void setTimeLimit(int milliseconds);
    // Zero means unlimited. Useful for reproducible engine comparisons.
    void setNodeLimit(uint64_t nodes);
    unsigned int getLastDepth() const;
    unsigned long long getLastNodes() const;
    int getLastScore() const;
    std::string getLastPV() const;
    int _evaluate() const;

    // Kept for the existing diagnostic API. solve() restores the loaded board.
    int _rootPlayer = 1;
    Rules::Board _board;

private:
    friend struct SolverTestAccess;
    static constexpr int MAX_PLY = 64;
    static constexpr int QUIESCENCE_DEPTH = 8;
    static constexpr int MATE_THRESHOLD = WIN - MAX_PLY;
    static constexpr int TABLE_SIZE = 1 << 18;
    struct Features {
        int score = 0;
        int fours = 0;
        int threes = 0;
        int captured = 0;
        bool five = false;
        bool captureSetup = false;
        bool legal = false;
    };
    struct Move {
        int index;
        int score;
        bool forcing;
        bool capture = false;
    };
    struct MoveList {
        std::vector<Move> moves;
        bool forced = false;
        bool lost = false;
        bool won = false;
    };
    struct Undo {
        int index;
        int player;
        std::array<int, 16> captured{};
        int count = 0;
        std::array<int, 2> capturesBefore;
    };
    enum Bound : uint8_t { EXACT, LOWER, UPPER };
    struct TableEntry {
        uint64_t key = 0;
        int score = 0;
        int16_t move = -1;
        int8_t depth = -1;
        Bound bound = EXACT;
    };
    struct Line {
        std::vector<int> cells;
        std::array<int, 2> scores{};
        std::array<int, 2> fives{};
    };

    mutable std::array<std::array<Features, CELLS>, 2> _featureCache{};
    mutable std::array<std::array<bool, CELLS>, 2> _featureDirty{};
    std::array<int, 2> _captures{};
    std::array<int, CELLS> _neighbors{};
    std::array<std::array<int, 4>, CELLS> _cellLines{};
    std::vector<Line> _lines;
    std::array<int, 2> _positionScore{};
    std::array<int, 2> _fives{};
    std::vector<Undo> _undo;
    std::vector<TableEntry> _table;
    std::array<std::array<int, CELLS>, 2> _history{};
    std::array<std::array<int, 2>, MAX_PLY> _killers{};
    uint64_t _boardHash = 0;
    int _stoneCount = 0;
    int _timeLimitMs = 2000;
    uint64_t _nodeLimit = 0;
    uint64_t _nodes = 0;
    bool _aborted = false;
    std::chrono::steady_clock::time_point _start;
    unsigned int _lastDepth = 0;
    unsigned long long _lastNodes = 0;
    int _lastScore = 0;
    std::string _lastPV;
    int _iterationMove = -1;

    void _load(const Rules::Board& board, int turn, const std::array<int, 2>& captures);
    void _setCell(int index, int value);
    void _refreshLines(const Undo& undo);
    void _scoreLine(int line);
    bool _play(int index, int player);
    void _unplay();
    Features _features(int index, int player) const;
    bool _wins(int index, int player, const Features& features);
    bool _hasImmediateWin(int player);
    int _winner(int lastPlayer) const;
    int _staticEval(int player) const;
    uint64_t _hash(int player) const;
    MoveList _generateMoves(int player, int ply);
    int _search(int depth, int ply, int alpha, int beta, int player, int qleft);
    bool _timeUp();
    void _buildPV(int player, unsigned int depth);
};
#endif
