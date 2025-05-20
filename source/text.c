#include "text.h"

const char* text_main_menu[] = {
    "START",
    "SETTINGS",
    "CREDITS"
};

const char* text_settings[] = {
    "SETTINGS",
    "FRAMERATE:",
    "VIDEO MODE:",
    "ASPECT RATIO:",
    "RES X:",
    "BACK",
};

const char* text_pause_menu[] = {
    "PAUSED",
    "CONTINUE",
    "SETTINGS",
    "EXIT GAME",
};

const char* text_credits[] = {
    "PlayStation 1 port",
    "Lily Haverlag",
    "",
    "",
    "--Original PC Team--",
    "",
    "",
    "Producer",
    "Tim van der Leeden",
    "",
    "",
    "Product Owner &",
    "Gameplay Programmer",
    "Ralph Warrand",
    "",
    "",
    "Gameplay Programmer",
    "Guy van der Meulen",
    "",
    "",
    "Engine & ",
    "Tools Programmers",
    "Wessel Frijters",
    "Stefan Claasse",
    "Sam van der Hoeven",
    "Joey Staps",
    "",
    "",
    "CICD & Graphics",
    "Programmer",
    "Melvin Rother",
    "",
    "",
    "Sound Designers &",
    "Graphics Programmers",
    "Daniel Cornelisse",
    "Lily Haverlag",
    "",
    "",
    "Narrative, System,",
    "and Level Designer",
    "Attila Szucs",
    "",
    "",
    "Level Designer",
    "Stefanos Zarakovitis",
    "Victor Stanciu",
    "",
    "",
    "Visual Artist",
    "Sasja Bradic",
    "Jesse Reichel",
    "",
    "",
    "Asset Credits",
    "Makkon3 Textures",
    "Ben Makkon Hale",
    "",
    "",
    "Special Thanks",
    "to Ex-devs",
    "Bart van Dongen",
    "Jonathan van der Heide",
    "Silvijn de Vries",
    "Cass van den Bos",
    "Saskia Janssen",
    "",
    "",
    "Special Thanks to Lecturers",
    "",
    "Nick Dry",
    "Bojan Endrovski",
    "David Rhodes",
    "Nicole de Jong",
};

const char* text_debug_menu_main[] = {
    "DEBUG",
    "MUSIC TEST",
    "LEVEL SELECT",
    "MAIN MENU"
};

const char* text_debug_menu_music[] = {
    "MUSIC TEST",
    "level1.dss",
    "level2.dss",
    "level3.dss",
    "subnivis.dss",
    "EXIT"
};

const char* text_debug_menu_level[] = {
    "LEVEL SELECT",
    "test.lvl",
    "test2.lvl",
    "test3.lvl",
    "level1.lvl",
    "level2.lvl",
    "EXIT"
};

int n_text_main_menu = sizeof(text_main_menu) / sizeof(const char*);
int n_text_settings = sizeof(text_settings) / sizeof(const char*);
int n_text_pause_menu = sizeof(text_pause_menu) / sizeof(const char*);
int n_text_credits = sizeof(text_credits) / sizeof(const char*);
int n_text_debug_menu_main = sizeof(text_debug_menu_main) / sizeof(const char*);
int n_text_debug_menu_music = sizeof(text_debug_menu_music) / sizeof(const char*);
int n_text_debug_menu_level = sizeof(text_debug_menu_level) / sizeof(const char*);