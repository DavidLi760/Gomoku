#ifndef UI_HPP
# define UI_HPP

# include <SFML/Graphics.hpp>
# include <vector>
# include <iostream>
# include <random>
# include <algorithm>
# include "Solver.hpp"
# include "Position.hpp"


class UI {
    private:
        sf::RenderWindow _window;

        unsigned int _windowHeight;
		unsigned int _windowWidth;
		float _circleSize;
		float _gridSize;
		float _gridThickness;

        sf::Font _fontTitle;
		sf::Font _fontMain;
		sf::Font _fontSmall;
		sf::Texture _texture_background;
		sf::Texture _texture_object;
		sf::Texture _texture_robot_off;
		sf::Texture _texture_robot_on;
		sf::Texture _texture_screen_happy;
		sf::Texture _texture_screen_unhappy;

		sf::Sprite _sprite_background;
		sf::Sprite _sprite_robot_off;
		sf::Sprite _sprite_robot_on;
		sf::Sprite _sprite_screen_happy;
		sf::Sprite _sprite_screen_unhappy;

		sf::Color _colorWhiteShadow;
		sf::Color _colorWhitePlatform;
		sf::Color _colorWhiteEdge;
		sf::Color _colorWhite;
		sf::Color _colorBlackShadow;
		sf::Color _colorBlackPlatform;
		sf::Color _colorBlackEdge;
		sf::Color _colorBlack;

        void _drawGridLine(const sf::Vector2f& a, const sf::Vector2f& b, float thickness, const sf::Color& color);
		void _drawGrid();
		void _drawBoard(const std::vector<std::vector<uint8_t>>& board, const History& lastMove);
		void _drawUI(std::array<int, 2>& captureStone, std::array<int, 2>& winnerRecord, bool ai, int turn);
		
    public:
        static const uint8_t SIZE = 19;
        enum Cell
		{
			EMPTY = 0,
			BLACK = 1,
			WHITE = 2
		};
		enum Mode
		{
			STONE = 0,
			SUGGESTION = 1,
			HISTORY = 2
		};
        UI();
        ~UI();

        sf::RenderWindow& getWindow();
        float getGridSize();
		void drawCircle(float x, float y, float radius, const sf::Color& color, const sf::Color& edgeColor);
		void drawStone(float x, float y, float radius, int turn, Mode mode = STONE);
        void drawText(const std::string& str, int size, float rel_x, float rel_y, const sf::Color& color, sf::Font& font);
		void draw(const std::vector<std::vector<uint8_t>>& board, std::array<int, 2>& captureStone, std::array<int, 2>& winnerRecord, bool ai, int turn, const Position& suggPos, int winner, const History& lastMove);
		void drawWinningText(const std::string& str, int size, float x, float y);
};

#endif