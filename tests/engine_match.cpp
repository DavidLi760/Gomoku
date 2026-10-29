#include "Solver.hpp"
#include "BaselineSolver.hpp"
#include "Rules.hpp"
#include <chrono>
#include <iostream>
#include <random>
using Board=Rules::Board;
#ifndef MATCH_TIME_MS
#define MATCH_TIME_MS 100
#endif
#ifndef MATCH_GAMES
#define MATCH_GAMES 12
#endif
int main(){
 int wins=0,losses=0,draws=0;
 Solver current;current.setTimeLimit(MATCH_TIME_MS);BaselineSolver baseline;
 for(int game=0;game<MATCH_GAMES;++game){
  Board b(19,std::vector<uint8_t>(19));std::array<int,2> captures{0,0};
  int player=1,winner=0,plies=0;const int currentColor=game%2+1;
  // Six legal, nearby opening seeds, each played with both colors.
  const int openings[6][6]={{9,9,10,10,8,10},{9,9,10,9,9,8},{9,9,10,10,8,8},
                          {9,9,9,10,10,9},{9,9,10,8,8,9},{9,9,8,8,10,8}};
  const auto& opening=openings[(game/2)%6];
  for(int k=0;k<3;++k){b[opening[2*k]][opening[2*k+1]]=player;player=3^player;}
  for(;plies<100 && !winner;++plies){
   auto start=std::chrono::steady_clock::now();
   Position m=player==currentColor?current.solve(b,player,captures):baseline.solve(b,player,captures);
   auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
   if(!Rules::isOnBoard(m.x,m.y)||b[m.x][m.y]||Rules::checkDoubleFreeThree(b,player,m.x,m.y)){
    std::cout<<"illegal game="<<game<<" player="<<player<<" move="<<m.x<<","<<m.y<<std::endl;
    winner=3^player;break;
   }
   if(elapsed>MATCH_TIME_MS+100)std::cout<<"budget_overrun engine="<<(player==currentColor?"new":"old")<<" ms="<<elapsed<<std::endl;
   b[m.x][m.y]=player;Rules::capture(b,m.x,m.y,player,captures);
   winner=Rules::checkWinning(b,player,captures);player=3^player;
  }
  if(!winner)++draws;else if(winner==currentColor)++wins;else ++losses;
  std::cout<<"game="<<game+1<<" new_color="<<currentColor<<" result="
           <<(!winner?"draw":winner==currentColor?"win":"loss")<<" plies="<<plies<<" score="<<wins<<"/"<<losses<<"/"<<draws<<std::endl;
 }
 std::cout<<"FINAL wins="<<wins<<" losses="<<losses<<" draws="<<draws<<std::endl;
}
