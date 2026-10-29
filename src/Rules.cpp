#include "Rules.hpp"

int Rules::checkWinning(const std::vector<std::vector<uint8_t>>& board, int player, const std::array<int, 2>& captureStone)
{
	if (captureStone[0] >= CAPTURE_TO_WIN) return 1;
	if (captureStone[1] >= CAPTURE_TO_WIN) return 2;

 const auto hasFive = [](const std::vector<std::vector<uint8_t>>& b, int color) {
  static const int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
  for (int x=0;x<SIZE;++x) for(int y=0;y<SIZE;++y) {
   if (b[x][y] != color) continue;
   for (const auto& d : dirs) {
    int k=1;
    for (;k<5;++k) {
     int nx=x+k*d[0],ny=y+k*d[1];
     if (!isOnBoard(nx,ny) || b[nx][ny]!=color) break;
    }
    if(k==5) return true;
   }
  }
  return false;
 };
 const int opponent = 3 ^ player;
 // A five from the previous turn wins if it survived the reply.
 if (hasFive(board, opponent)) return opponent;
 if (!hasFive(board, player)) return 0;
 // A newly formed five wins unless a legal capture can defeat it
 // (or the opponent can capture their tenth stone).
 static const int dirs[4][2]={{1,0},{0,1},{1,1},{1,-1}};
 for (int x=0;x<SIZE;++x) for(int y=0;y<SIZE;++y) {
  if(board[x][y]) continue;
  bool canCapture=false;
  for(const auto& d:dirs) for(int sign:{-1,1}) {
   int nx=x+3*sign*d[0],ny=y+3*sign*d[1];
   if(isOnBoard(nx,ny) && board[nx][ny]==opponent &&
      board[x+sign*d[0]][y+sign*d[1]]==player &&
      board[x+2*sign*d[0]][y+2*sign*d[1]]==player) canCapture=true;
  }
  if(!canCapture) continue;
  auto reply=board;
  reply[x][y]=opponent;
  if(checkDoubleFreeThree(reply,opponent,x,y)) continue;
  auto captures=captureStone;
  if(capture(reply,x,y,opponent,captures) &&
     (captures[opponent-1]>=CAPTURE_TO_WIN || !hasFive(reply,player))) return 0;
 }
 return player;
}

bool Rules::capture(std::vector<std::vector<uint8_t>>& board, int x, int y, int turn, std::array<int, 2>& captureStone, std::array<int, 2>* captured)
{
	bool any = false;
	if (captured != nullptr) *captured = {-1, -1};

	const int directions[8][2] = {{1, 0}, {-1, 0},{0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
	for (const auto& dir : directions)
	{
		int dx = dir[0], dy = dir[1];
		int x3 = x + 3 * dx, y3 = y + 3 * dy;

		if (!isOnBoard(x3, y3)) continue;

		if (board[x3][y3] == turn && board[x + dx][y + dy] == (3 ^ turn) && board[x + 2 * dx][y + 2 * dy] == (3 ^ turn))
		{
			if (captured != nullptr && !any) *captured = {x + dx, y + dy};

			board[x + dx][y + dy] = 0;
			board[x + 2 * dx][y + 2 * dy] = 0;
			captureStone[turn - 1] += 2;
			any = true;
		}
	}

	return any;
}

bool Rules::isOnBoard(int x, int y)
{
	return x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}

bool Rules::checkDoubleFreeThree(const std::vector<std::vector<uint8_t>>& board, int turn, int lastX, int lastY)
{
 static const int dirs[4][2]={{1,0},{0,1},{1,1},{1,-1}};
 int threes=0;
 for(const auto& d:dirs) {
  bool freeThree=false;
  // A free three can become an open contiguous four in one move.
  for(int start=-3;start<=0 && !freeThree;++start) {
   int beforeX=lastX+(start-1)*d[0], beforeY=lastY+(start-1)*d[1];
   int afterX=lastX+(start+4)*d[0], afterY=lastY+(start+4)*d[1];
   if(!isOnBoard(beforeX,beforeY)||!isOnBoard(afterX,afterY)||
      board[beforeX][beforeY]||board[afterX][afterY]) continue;
   int stones=0,empty=0;
   for(int k=start;k<start+4;++k) {
    int nx=lastX+k*d[0],ny=lastY+k*d[1];
    if(nx==lastX && ny==lastY) ++stones;
    else if(board[nx][ny]==turn) ++stones;
    else if(board[nx][ny]==0) ++empty;
   }
   freeThree=stones==3 && empty==1;
  }
  if(freeThree && ++threes>=2) return true;
 }
 return false;
}

