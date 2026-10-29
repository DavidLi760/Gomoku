#include "Rules.hpp"
#include "Solver.hpp"
#include <cassert>
#include <iostream>
#include <chrono>
#include <random>
#include <sstream>
#include <stdexcept>
#include <algorithm>
using Board=std::vector<std::vector<uint8_t>>;
Board empty(){return Board(19,std::vector<uint8_t>(19));}

struct SolverTestAccess {
 static void compare(Solver& ai, const Rules::Board& b, const std::array<int,2>& captures) {
  Solver reference;
  reference._load(b,1,captures);
  assert(ai._board==b);
  assert(ai._captures==captures);
  assert(ai._boardHash==reference._boardHash);
  assert(ai._neighbors==reference._neighbors);
  assert(ai._fives==reference._fives);
  assert(ai._positionScore==reference._positionScore);
  for(int i=0;i<361;++i) for(int color:{1,2}) {
   auto actual=ai._features(i,color),expected=reference._features(i,color);
   assert(actual.score==expected.score && actual.five==expected.five &&
          actual.fours==expected.fours && actual.threes==expected.threes &&
          actual.captured==expected.captured && actual.legal==expected.legal);
   if(!b[i/19][i%19])
    assert(actual.legal==!Rules::checkDoubleFreeThree(b,color,i/19,i%19));
  }
 }
 static void verifyDefenses(const Board& b, int player) {
  Solver ai;ai._load(b,player,{0,0});
  ai._timeLimitMs=10000;ai._start=std::chrono::steady_clock::now();
  const auto actual=ai._generateMoves(player,0);
  assert(!ai._aborted && actual.forced && !actual.won);
  std::vector<int> expected,received;
  for(int i=0;i<361;++i) {
   if(!ai._neighbors[i] || !ai._play(i,player)) continue;
   int winner=ai._winner(player);
   bool safe=winner==player || (!winner && !ai._hasImmediateWin(3^player));
   ai._unplay();if(safe) expected.push_back(i);
  }
  for(const auto& move:actual.moves)received.push_back(move.index);
  std::sort(received.begin(),received.end());assert(expected==received);
 }
 static void run() {
  Solver ai;std::mt19937 random(42);
  auto b=empty();std::array<int,2> c{0,0};ai._load(b,1,c);
  std::vector<Board> boards{b};std::vector<std::array<int,2>> captures{c};
  for(int ply=0;ply<70;++ply) {
   int color=ply%2+1;
   int index;
   do {index=4*19+4+static_cast<int>(random()%(11*19+11));}
   while(b[index/19][index%19] || Rules::checkDoubleFreeThree(b,color,index/19,index%19));
   assert(ai._play(index,color));
   b[index/19][index%19]=color;Rules::capture(b,index/19,index%19,color,c);
   compare(ai,b,c);boards.push_back(b);captures.push_back(c);
  }
  for(int ply=69;ply>=0;--ply) {
   ai._unplay();compare(ai,boards[ply],captures[ply]);
  }
  // The largest simultaneous capture removes sixteen stones.
  b=empty();c={0,0};
  static const int d[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,-1},{1,-1},{-1,1}};
  for(const auto& dir:d) {
   b[9+dir[0]][9+dir[1]]=b[9+2*dir[0]][9+2*dir[1]]=2;
   b[9+3*dir[0]][9+3*dir[1]]=1;
  }
  auto before=b;ai._load(b,1,c);assert(ai._play(9*19+9,1));
  b[9][9]=1;Rules::capture(b,9,9,1,c);assert(c[0]==16);compare(ai,b,c);
  ai._unplay();compare(ai,before,{0,0});
  for(int color:{1,2}) {
   // Quietly establishing a capture anchor is also a valid defense.
   b=empty();for(int x=5;x<=8;++x)b[x][9]=3^color;
   b[4][9]=color;b[7][10]=3^color;verifyDefenses(b,color);
   // A pending five can be broken by simultaneous captures on two diagonals.
   b=empty();for(int x=7;x<=11;++x)b[x][9]=3^color;
   b[7][10]=b[11][10]=3^color;b[6][11]=b[12][11]=color;
   verifyDefenses(b,color);
  }
 }
};

void advancedTests() {
 SolverTestAccess::run();
 Solver ai;ai.setTimeLimit(5000);ai.setNodeLimit(5000);
 for(int color:{1,2}) {
  auto b=empty();b[5][9]=b[6][9]=b[8][9]=b[9][9]=color;
  auto p=ai.solve(b,color,{0,0});assert(p.x==7&&p.y==9);
  // An open four is a win in three plies, not just a high heuristic score.
  b=empty();b[7][9]=b[8][9]=b[9][9]=color;b[0][0]=3^color;
  p=ai.solve(b,color,{0,0});assert(p.y==9&&(p.x==6||p.x==10));
  assert(ai.getLastScore()>Solver::WIN-64);
  // One diagonal capture defuses two closed fours on different axes.
  b=empty();for(int x=5;x<=8;++x)b[x][9]=3^color;
  for(int y=7;y<=10;++y)b[8][y]=3^color;
  b[4][9]=b[8][6]=b[9][11]=color;
  p=ai.solve(b,color,{0,0});
  assert(p.x==6&&p.y==8);
  assert(ai._board==b);
 }
 auto b=empty();b[8][9]=b[9][8]=1;b[9][9]=b[10][9]=b[10][10]=2;
 ai.setNodeLimit(1000);auto first=ai.solve(b,1,{2,4});auto score=ai.getLastScore();auto pv=ai.getLastPV();
 auto second=ai.solve(b,1,{2,4});
 assert(first.x==second.x&&first.y==second.y&&score==ai.getLastScore()&&pv==ai.getLastPV());
 // Recorded PV contains legal moves in their actual positions.
 std::istringstream stream(ai.getLastPV());int x,y,color=1;char comma;std::array<int,2> captures{2,4};
 while(stream>>x>>comma>>y) {
  assert(comma==','&&Rules::isOnBoard(x,y)&&!b[x][y]);
  assert(!Rules::checkDoubleFreeThree(b,color,x,y));b[x][y]=color;
  Rules::capture(b,x,y,color,captures);color=3^color;
 }
 // A tiny budget still returns a legal fallback and restores the board.
 ai.setNodeLimit(0);ai.setTimeLimit(1);
 auto start=std::chrono::steady_clock::now();auto p=ai.solve(b,color,captures);
 auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
 assert(ms<150);
 if(p.x>=0) assert(!b[p.x][p.y]&&!Rules::checkDoubleFreeThree(b,color,p.x,p.y));
 assert(ai._board==b);
 bool invalid=false;
 try{ai.solve({},1,{0,0});}catch(const std::invalid_argument&){invalid=true;}
 assert(invalid);
 std::cout<<"Incremental state, forced lines, captures, deterministic budget and deadline tests passed\n";
}

int main(){
 auto b=empty();std::array<int,2> c{0,0};
 b[9][9]=1;b[10][9]=b[11][9]=2;b[12][9]=1;
 b[9][10]=b[9][11]=2;b[9][12]=1;
 assert(Rules::capture(b,9,9,1,c));assert(c[0]==4);
 assert(!b[10][9]&&!b[11][9]&&!b[9][10]&&!b[9][11]);
 b=empty();c={0,0};for(int x=5;x<10;++x)b[x][9]=1;
 assert(Rules::checkWinning(b,1,c)==1);
 b[7][10]=1;b[7][11]=2;assert(Rules::checkWinning(b,1,c)==0);
 b=empty();b[8][9]=b[10][9]=b[9][8]=b[9][10]=1;
 assert(Rules::checkDoubleFreeThree(b,1,9,9));
 b[7][9]=2;assert(!Rules::checkDoubleFreeThree(b,1,9,9));
 Solver ai;b=empty();auto p=ai.solve(b,1,{0,0});assert(p.x==9&&p.y==9);
 for(int color:{1,2})for(int axis=0;axis<4;++axis){
 static const int d[4][2]={{1,0},{0,1},{1,1},{1,-1}};
 b=empty();for(int k=0;k<4;++k)b[5+k*d[axis][0]][9+k*d[axis][1]]=color;
 auto original=b;p=ai.solve(b,color,{0,0});assert(p.x>=0&&p.y>=0&&!b[p.x][p.y]);
 assert(ai._board==original);b[p.x][p.y]=color;
 assert(Rules::checkWinning(b,color,{0,0})==color);
 }
 b=empty();b[4][9]=1;for(int x=5;x<9;++x)b[x][9]=2;
 p=ai.solve(b,1,{0,0});assert(p.x==9&&p.y==9);
 b=empty();b[10][9]=b[11][9]=2;b[12][9]=1;
 p=ai.solve(b,1,{8,0});b[p.x][p.y]=1;c={8,0};Rules::capture(b,p.x,p.y,1,c);assert(c[0]>=10);
 advancedTests();
 std::cout<<"All tactical tests passed\n";
}
