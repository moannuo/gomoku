#include "Game.hpp"
#include "BitBoard.hpp" // DEBUG

Game::Game(void)  {
    this->_game_engine = new GameEngine();
    this->_gui = new GraphicalInterface(this->_game_engine);
    // this->_config = this->_gui->render_choice_menu();
    this->_config = "p1=C,p2=C,nu=1,db=1,sg=1";
    this->_configure();
}

Game::Game(Game const &src) {
    *this = src;
}

Game::~Game(void) {
    delete this->_player_1;
    delete this->_player_2;
    delete this->_game_engine;
    delete this->_gui;
}

Game    &Game::operator=(Game const &src) {
    this->_player_1 = src.get_player_1();
    this->_player_2 = src.get_player_2();
    this->_game_engine = src.get_game_engine();
    this->_gui = src.get_gui();
    return (*this);
}

void    Game::_configure(void) {
    this->_player_1 = ( this->_config[this->_config.find("p1=")+3]=='H' ? (Player*)new Human(this->_game_engine, this->_gui, 1) : (Player*)new Computer(this->_game_engine, this->_gui, 1) );
    this->_player_2 = ( this->_config[this->_config.find("p2=")+3]=='H' ? (Player*)new Human(this->_game_engine, this->_gui, 2) : (Player*)new Computer(this->_game_engine, this->_gui, 2) );
    this->_gui->set_nu((this->_config[this->_config.find("nu=")+3]=='1' ? true : false));
    this->_gui->set_db((this->_config[this->_config.find("db=")+3]=='1' ? true : false));
    this->_gui->set_sg((this->_config[this->_config.find("sg=")+3]=='1' ? true : false));
    this->_c_player = this->_player_1;
    this->_gui->get_analytics()->set_players(this->_c_player, this->_player_1, this->_player_2);
}

void    Game::_debug_fps(void) {
    static int32_t frames = -1;
    static uint32_t ms = this->_gui->get_analytics()->get_chronometer()->get_elapsed_ms();

    frames++;
    if (this->_gui->get_analytics()->get_chronometer()->get_elapsed_ms() - ms >= 1000) {
        std::cout << "fps : " << frames << std::endl;
        ms = this->_gui->get_analytics()->get_chronometer()->get_elapsed_ms();
        frames = 0;
    }
}

void    Game::_cap_framerate(uint32_t const &framerate) {
    static double last = this->_gui->get_analytics()->get_chronometer()->get_elapsed_ms();
    double delta;

    delta = std::abs(this->_gui->get_analytics()->get_chronometer()->get_elapsed_ms() - last);
    if (delta < (1000. / framerate))
        std::this_thread::sleep_for(std::chrono::milliseconds((uint64_t)(1000. / framerate - delta)));
    last = this->_gui->get_analytics()->get_chronometer()->get_elapsed_ms();
}

void    Game::loop(void) {
    bool    action_performed;
    bool    action_undo;

    while (true) {
        action_undo = false;
        action_performed = false;
        this->_gui->update_events();
        if (this->_gui->check_pause() == false)
            action_performed = this->_c_player->play(this->_c_player->get_id() == 1 ? this->_player_2 : this->_player_1);
        if (this->_gui->check_undo()) {
            action_undo = true;
            if (undo()) continue;
        }
        if (this->_gui->check_newgame())
            newgame();
        if (this->_gui->check_restart())
            restart();
        if (this->_gui->check_close())
            break;
        if (action_undo == false)
            this->_gui->update_end_game(*this->_c_player, this->_c_player->get_id() == 1 ? *this->_player_2 : *this->_player_1);
        if ((action_performed == true && !this->_gui->check_pause()) || (action_undo == true && !this->_gui->get_end_game())) {
            // std::cout << "________________________\n" << this->_c_player->board << std::endl;
            // BitBoard    test;
            // test |= highlight_captured_stones(p1_tmp, p2_tmp, p);
            // test |= win_by_capture_detector(this->_c_player->board, (this->_c_player->get_id() == 1 ? this->_player_2->board : this->_player_1->board), this->_c_player->get_pairs_captured());
            // test |= pair_capture_breaking_five_detector(this->_c_player->board, (this->_c_player->get_id() == 1 ? this->_player_2->board : this->_player_1->board));
            // test |= get_winning_moves_debug(this->_c_player->board, (this->_c_player->get_id() == 1 ? this->_player_2->board : this->_player_1->board), this->_c_player->get_pairs_captured(), (this->_c_player->get_id() == 1 ? this->_player_2->get_pairs_captured() : this->_player_1->get_pairs_captured()));
            // test |= future_pattern_detector(this->_c_player->board, (this->_c_player->get_id() == 1 ? this->_player_2->board : this->_player_1->board), { 0xF8, 5, 8, 0 });
            // test |= pattern_detector_highlight_open(this->_c_player->board, (this->_c_player->get_id() == 1 ? this->_player_2->board : this->_player_1->board), { 0x68, 6, 8, 0 }); // OO-OO 0x6C is old
            // test |= highlight_five_aligned(this->_c_player->board);
            this->_c_player = (this->_c_player->get_id() == 1 ? this->_player_2 : this->_player_1); /* switch players */
        }
        this->_gui->get_analytics()->set_c_player(this->_c_player);
        this->_gui->update_display();
        // this->_debug_fps();
        this->_cap_framerate(30);
    }
}

bool    Game::undo(void) {
    bool    last = (this->_game_engine->get_history_size() == 0);
    this->_game_engine->delete_last_action(this->_player_1, this->_player_2);
    this->_gui->explored_moves_tmp = moves_to_explore(
        this->_c_player->board,
        (this->_c_player->get_id() == 1 ? this->_player_2 : this->_player_1)->board,
        this->_c_player->board_forbidden,
        this->_c_player->get_pairs_captured(),
        (this->_c_player->get_id() == 1 ? this->_player_2 : this->_player_1)->get_pairs_captured()
    );
    return (last);
}

/*  Restart only reset the game to 0 with the same configuration, while
    New Game displays the new game menu to configure the game.
*/
void    Game::restart(void) {
    delete this->_gui;
    delete this->_game_engine;
    delete this->_player_1;
    delete this->_player_2;

    this->_game_engine = new GameEngine();
    this->_gui = new GraphicalInterface(this->_game_engine);
    this->_configure();
    Game::loop();
}

void    Game::newgame(void) {
    delete this->_gui;
    delete this->_game_engine;
    delete this->_player_1;
    delete this->_player_2;

    this->_game_engine = new GameEngine();
    this->_gui = new GraphicalInterface(this->_game_engine);
    this->_config = this->_gui->render_choice_menu();
    this->_configure();
    Game::loop();
}
