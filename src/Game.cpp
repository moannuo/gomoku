#include "Game.hpp"

Game::Game(void)  {
    // this->_input_handler = new InputHandler();
    // std::vector<std::string>    players = this->_input_handler.set_players();

    this->_game_engine = new GameEngine();

    /* Debug */
    this->_current_player = 1;
    this->_player_1 = new Human(this->_game_engine, 0);
    this->_player_2 = new Computer(this->_game_engine, 1);
}

Game::Game(Game const &src) {
    *this = src;
}

Game::~Game(void) {
    delete this->_player_1;
    delete this->_player_2;
    delete this->_game_engine;
}

Game	&Game::operator=(Game const &src) {
    this->_player_1 = src.get_player_1();
    this->_player_2 = src.get_player_2();
    this->_game_engine = src.get_game_engine();
    return (*this);
}

/* Getters */
Player      *Game::get_player_1(void) const { return (this->_player_1); }
Player      *Game::get_player_2(void) const { return (this->_player_2); }
GameEngine  *Game::get_game_engine(void) const { return (this->_game_engine); }
/* Setters */
void    Game::set_player_1(Player player) { this->_player_1 = &player; }
void    Game::set_player_2(Player player) { this->_player_2 = &player; }


void    Game::loop(void) const {
    // while (true) {
    //     this->_current_player = (this->_current_player == 1 ? 0 : 1); // switch the current player
    //     this->_current_player->play();
    //     if (this->_game_engine->check_end() == true)
    //         break;
    // }
}

void    Game::end(void) const {

}
