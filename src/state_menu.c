#include "common.h"
#include "state_game.h"

static void menu_init(void)
{
    gfx_FillScreen(0xFF);

    gfx_SetTextScale(3, 3);
    printCentered("STACK", 50);

    gfx_SetTextScale(1, 1);
    printCentered("Press <enter> to start", 80);
    printCentered("Press <clear> to exit", 90);

    printCentered("made with <3 by voltie_dev", GFX_LCD_HEIGHT - 20);

    gfx_SwapDraw();
}

static void menu_step(void)
{
    if (keyPressed(kb_KeyClear))
    {
        is_running = false;
    }

    if (keyPressed(kb_KeyEnter))
    {
        enterState(STATE_GAME);
    }
}

static void menu_draw(void)
{
}

const GameState STATE_MENU = {menu_init, menu_step, menu_draw};