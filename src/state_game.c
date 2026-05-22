#include "common.h"

static void game_init(void)
{
}

static void game_step(void)
{
}

static void game_draw(void)
{
    gfx_PrintStringXY("gamee!!", 10, 10);
}

const GameState STATE_GAME = {game_init, game_step, game_draw};