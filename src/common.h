#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdbool.h>
#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>

#define HIGH_SCORE_APPVAR_NAME "STACKHI"

typedef void (*StateFunc)(void);

typedef struct
{
    StateFunc init;
    StateFunc step;
    StateFunc draw;
} GameState;

void enterState(GameState new_state);
bool keyPressed(kb_lkey_t key);
void printCentered(const char *str, int y);

extern void saveHighScore(uint32_t current_score);
extern uint32_t loadHighScore(void);

extern GameState current_state;
extern bool is_running;

#endif