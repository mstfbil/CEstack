#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <graphx.h>
#include <keypadc.h>

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

extern GameState current_state;
extern bool is_running;

#endif