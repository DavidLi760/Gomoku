# include "UI.hpp"

UI::UI(): 
	_fontTitle("assets/fonts/SpecialElite-Regular.ttf"),
	_fontMain("assets/fonts/Comfortaa-VariableFont_wght.ttf"),
	_fontSmall("assets/fonts/Tektur-VariableFont_wdth,wght.ttf"),

	_texture_background("assets/images/texture_board.png"),
	_texture_object("assets/images/texture_board_square.jpg"),
	_texture_robot_off("assets/images/robot/off.png"),
	_texture_robot_on("assets/images/robot/on.png"),
	_texture_screen_happy("assets/images/robot/screen_happy.png"),
	_texture_screen_unhappy("assets/images/robot/screen_unhappy.png"),

	_sprite_background(_texture_background),
	_sprite_robot_off(_texture_robot_off),
	_sprite_robot_on(_texture_robot_on),
	_sprite_screen_happy(_texture_screen_happy),
	_sprite_screen_unhappy(_texture_screen_unhappy)
{
    _windowHeight = 600;
    _windowWidth = 800;
    _circleSize = 0.85 * 0.5 * _windowHeight / (SIZE + 2);
    _gridSize = _windowHeight / (SIZE + 2);
	_gridThickness = 0.2;

	_colorWhiteShadow = sf::Color(200, 200, 200);
	_colorWhiteEdge = sf::Color(210, 210, 210);
	_colorWhitePlatform = sf::Color(230, 230, 230);
	_colorWhite = sf::Color(250, 250, 250);
	_colorBlackShadow = sf::Color(50, 50, 50);
	_colorBlackEdge = sf::Color(70, 70, 70);
	_colorBlackPlatform = sf::Color(100, 100, 100);
	_colorBlack = sf::Color(130, 130, 130);

	_sprite_background.setScale({0.57f, 0.55f});
	_sprite_robot_off.setPosition({1.18f * _windowHeight - 15.f, 0.72f * _windowHeight - 25.f});
	_sprite_robot_off.setScale({52.f / _texture_robot_off.getSize().x, 52.f / _texture_robot_off.getSize().y});
	_sprite_robot_on.setPosition({1.18f * _windowHeight - 15.f, 0.72f * _windowHeight - 25.f});
	_sprite_robot_on.setScale({52.f / _texture_robot_on.getSize().x, 52.f / _texture_robot_on.getSize().y});

    _window = sf::RenderWindow(sf::VideoMode({_windowWidth, _windowHeight}), "Gomoku");
    _window.setFramerateLimit(10);
}

UI::~UI(){

}

sf::RenderWindow& UI::getWindow(){
    return this->_window;
}

float UI::getGridSize(){
    return this->_gridSize;
}

void UI::_drawGridLine(const sf::Vector2f& a, const sf::Vector2f& b, float thickness, const sf::Color& color){
	sf::Vector2f d = b - a;
	float len = std::sqrt(d.x * d.x + d.y * d.y);
	float ang = std::atan2(d.y, d.x) * 180.f / 3.14159265f;

	sf::RectangleShape r({len, thickness});
	r.setOrigin({0.0f, thickness / 2.f});
	r.setPosition(a);
	r.setRotation(sf::degrees(ang));
	r.setFillColor(color);
	_window.draw(r);
}

void UI::_drawGrid(){
    for (size_t i = 0; i < SIZE; i++){
		_drawGridLine({_gridSize * (i + 1.5f), _gridSize * 1.5f}, 
					  {_gridSize * (i + 1.5f), _gridSize * (SIZE + 0.5f)}, 2.f, sf::Color(100, 65, 30));
	}
	for (size_t i = 0; i < SIZE; i++){
		_drawGridLine({_gridSize * 1.5f, _gridSize * (i + 1.5f)}, 
					  {_gridSize * (SIZE + 0.5f), _gridSize * (i + 1.5f)}, 2.f, sf::Color(100, 65, 30));
	}
}

void UI::_drawBoard(const  std::vector<std::vector<uint8_t>>& board, const History& lastMove){
    _drawGrid();

	for (size_t i = 0; i < board.size(); i++) {
		for (size_t j = 0; j < board[i].size(); j++){
			if (board[i][j] == 0){
				continue;
			}
			bool isLastMove = static_cast<int>(i) == lastMove.x
				&& static_cast<int>(j) == lastMove.y && board[i][j] == lastMove.player;
			drawStone(_gridSize * (i + 1) + 1.f, _gridSize * (j + 1) + 1.f,
				_circleSize, isLastMove ? lastMove.player : board[i][j], isLastMove ? HISTORY : STONE);
		}
	}
}

		
void UI::_drawUI(std::array<int, 2>& captureStone, std::array<int, 2>& winnerRecord, bool ai, int turn){

	float recMidX = _windowHeight + (_windowWidth - _windowHeight) * 0.4f + 10.f;

	// Title
	drawText("Gomoku", 45, -0.055f, 0.11, sf::Color(0, 0, 0), _fontTitle);
	drawText("Gomoku", 45, -0.05f, 0.105, sf::Color(173, 156, 144), _fontTitle);

	// Turn
	drawText((turn == 1 ? std::string("White") : std::string("Black")) + std::string("'s turn"), 15, 0.f, 0.26f, sf::Color(0, 0, 0), _fontMain);
	drawStone(recMidX - 2.f * _gridSize, 0.3f * _windowHeight, _circleSize, turn);

	// Capture
	drawText(std::string("Capture stone\n"), 15, 0.f, 0.37f, sf::Color(0, 0, 0), _fontMain);
	drawText(std::to_string(captureStone[0]) + std::string("  :  ") + std::to_string(captureStone[1]), 35, 0.1f, 0.40f, sf::Color(0, 0, 0), _fontSmall);
	
	// Winner
	drawText(std::string("Score\n"), 15, 0.f, 0.50f, sf::Color(0, 0, 0), _fontMain);
	drawText(std::to_string(winnerRecord[0]) + std::string("  :  ") + std::to_string(winnerRecord[1]), 35, 0.1f, 0.53f, sf::Color(0, 0, 0), _fontSmall);

	drawStone(recMidX - 2.f * _gridSize, 0.62f * _windowHeight, _circleSize, 1);
	drawStone(recMidX + 0.f * _gridSize, 0.62f * _windowHeight, _circleSize, 2);

	// AI
	drawText(std::string("AI ") + (ai ? std::string("Enabled") : std::string("Disabled")), 15, 0.f, 0.72f, sf::Color(0, 0, 0), _fontMain);
	
	if (ai){
		_window.draw(_sprite_robot_on);
	} else {
		_window.draw(_sprite_robot_off);
	}
	// Notice
	drawText("R to reset game\nA to enable AI\nS for suggestion", 15, 0.f, 0.78, sf::Color(0, 0, 0), _fontMain);
}


void UI::drawCircle(float x, float y, float radius, const sf::Color& color, const sf::Color& edgeColor){
    sf::CircleShape shape(radius);
	shape.setPosition({x, y});
	shape.setOutlineThickness(2.f);
	shape.setOutlineColor(edgeColor);	
	shape.setFillColor(color);
	_window.draw(shape);
}


void UI::drawStone(float x, float y, float radius, int turn, Mode mode){
	if (mode == SUGGESTION) {
		sf::Color color = turn == 1 ? _colorWhite : _colorBlackShadow;
		color.a = 65;
		drawCircle(x, y, radius, color, color);
		return;
	}
	drawCircle(x + 2.f, y + 2.f, _circleSize, sf::Color(30, 30, 30), sf::Color(30, 30, 30));
    if (turn == 1) {
        drawCircle(x, y, radius, _colorWhiteShadow, _colorWhiteShadow);
		drawCircle(x + radius * 0.25, y + radius * 0.25, radius * 0.75, _colorWhitePlatform, _colorWhitePlatform);
		drawCircle(x + radius* 0.4, y + radius * 0.4, radius * 0.3, _colorWhite, _colorWhite);
    } else if (turn == 2) {
        drawCircle(x, y, radius, _colorBlackShadow, _colorBlackShadow);
		drawCircle(x + radius * 0.25, y + radius * 0.25, radius * 0.75, _colorBlackPlatform, _colorBlackPlatform);
		drawCircle(x + radius * 0.4, y + radius * 0.4, radius * 0.3, _colorBlack, _colorBlack);
    }
	if (mode == HISTORY) {
		sf::CircleShape frame(radius);
		frame.setPosition({x, y});
		frame.setFillColor(sf::Color::Transparent);
		frame.setOutlineColor(sf::Color(190, 100, 100));
		frame.setOutlineThickness(2.f);
		_window.draw(frame);
	}
}
		
void UI::drawText(const std::string& str, int size, float rel_x, float rel_y, const sf::Color& color, sf::Font& font){
    sf::Text text = sf::Text(font, str, size);

	text.setFillColor(color);
	text.move({static_cast<float>(_windowHeight + rel_x * (_windowWidth - _windowHeight) + 10.f), 
				static_cast<float>(rel_y * _windowHeight)});
	_window.draw(text);
}

void UI::drawWinningText(const std::string& str, int size, float x, float y){
    sf::Text text = sf::Text(_fontSmall, str, size);

	text.setFillColor(sf::Color(25, 25, 25));
	text.move({x, y});
	_window.draw(text);
	text.setFillColor(sf::Color(100, 245, 255));
	text.move({-1.f, -1.f});
	_window.draw(text);
}
		
void UI::draw(const std::vector<std::vector<uint8_t>>& board,
        std::array<int, 2>& captureStone,
		std::array<int, 2>& winnerRecord,
        bool ai, int turn, const Position& suggPos, int winner, const History& lastMove){
   	
	// Texture
	_window.draw(_sprite_background);
	sf::RectangleShape rectangle({0.91f * _windowHeight, 0.91f * _windowHeight});
	rectangle.setPosition({0.04f * _windowHeight, 0.04f * _windowHeight});
	rectangle.setFillColor(sf::Color(70, 70, 70));
	_window.draw(rectangle);
	rectangle.setFillColor(sf::Color::White);
	rectangle.setTexture(&_texture_object);
	rectangle.setPosition({0.035f * _windowHeight, 0.035f * _windowHeight});
	_window.draw(rectangle);


    _drawBoard(board, lastMove);
	_drawUI(captureStone, winnerRecord, ai, turn);

    // Draw suggestion
    if (!ai && suggPos.x >= 0){
		drawStone(_gridSize * (suggPos.x + 1) + 1.f, _gridSize * (suggPos.y + 1) + 1.f, _circleSize, turn, SUGGESTION);
    }

	if (winner != 0){
		std::string winningMessage = ai
			? (winner == 1 ? "You win!" : "AI win!")
			: (winner == 1 ? "White win!" : "Black win!");

		int textSize = 50;
		float textX = 0.5f * _windowWidth - 6 * textSize * 0.5f;
		float textY = 0.5f * _windowHeight - textSize * 0.5f;
		sf::Text text(_fontSmall, winningMessage, textSize);
		sf::FloatRect bounds = text.getLocalBounds();
		// Fit the text into the empty screen area below the robot's face.
		float scale = std::max((bounds.size.x + 48.f) / 740.f,
			(bounds.size.y + 32.f) / 170.f);
		sf::Sprite& robot = (!ai || winner == 1) ? _sprite_screen_happy : _sprite_screen_unhappy;
		robot.setScale({scale, scale});
		robot.setPosition({textX + bounds.position.x + bounds.size.x * 0.5f - 627.f * scale,
			textY + bounds.position.y + bounds.size.y * 0.5f - 710.f * scale});
		_window.draw(robot);
		drawWinningText(winningMessage, textSize, textX, textY);
	}
	
	_window.display();
}
