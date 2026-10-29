#include "Gomoku.hpp"

int main(int ac, char** av){
    Gomoku gomoku = Gomoku();
    
    if (ac >= 2 && std::string(av[1]) == "--ai"){
        gomoku.useAI(true);
    }
    
    gomoku.run();

    return 0;
}