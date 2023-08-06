#ifndef MAIN_H
#define MAIN_H

int main(void);
void init(void);

typedef enum {
    STATE_NONE,
    STATE_TITLE_SCREEN,
    STATE_SETTINGS,
    STATE_CREDITS,
    STATE_IN_GAME,
    STATE_PAUSE_MENU,
} state_t;

// Title screen
void state_enter_title_screen(void);
void state_update_title_screen(int dt);
void state_exit_title_screen(void);

// Settings
void state_enter_settings(void);
void state_update_settings(int dt);
void state_exit_settings(void);

// Credits
void state_enter_credits(void);
void state_update_credits(int dt);
void state_exit_credits(void);

// In game
void state_enter_in_game(void);
void state_update_in_game(int dt);
void state_exit_in_game(void);

// Pause menu
void state_enter_pause_menu(void);
void state_update_pause_menu(int dt);
void state_exit_pause_menu(void);

#endif