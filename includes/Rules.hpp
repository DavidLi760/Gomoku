#ifndef RULES_HPP
#define RULES_HPP

#include <array>
#include <cstdint>
#include <vector>

namespace Rules {
using Board = std::vector<std::vector<uint8_t>>;
inline constexpr int SIZE = 19;
inline constexpr int CAPTURE_TO_WIN = 10;
bool isOnBoard(int x, int y);
bool checkDoubleFreeThree(const Board& board, int player, int x, int y);
bool capture(Board& board, int x, int y, int player, std::array<int, 2>& captures,
             std::array<int, 2>* firstCaptured = nullptr);
int checkWinning(const Board& board, int lastPlayer, const std::array<int, 2>& captures);
}
#endif
