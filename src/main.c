#include "common.h"
#include "state_menu.h"
#include "state_game.h"

#include <debug.h>

GameState current_state;
bool is_running = true;

static uint8_t old_kb_Data[8];

void enterState(GameState new_state)
{
    current_state = new_state;

    if (current_state.init != NULL)
        current_state.init();
}

void inputUpdate(void)
{
    for (int i = 0; i <= 7; i++)
    {
        old_kb_Data[i] = kb_Data[i];
    }
    kb_Scan();
}

bool keyPressed(kb_lkey_t key)
{
    uint8_t group = (uint8_t)(key >> 8);
    uint8_t mask = (uint8_t)(key & 0xFF);
    dbg_printf("group %i, mask %i\n", group, mask);
    return (kb_Data[group] & mask) && !(old_kb_Data[group] & mask);
}

void printCentered(const char *str, int y)
{
    int x = (GFX_LCD_WIDTH - gfx_GetStringWidth(str)) / 2;
    gfx_PrintStringXY(str, x, y);
}

int main(void)
{
    gfx_Begin();
    gfx_SetDrawBuffer();

    enterState(STATE_GAME);

    while (is_running)
    {
        inputUpdate();
        current_state.step();
        if (is_running)
            current_state.draw();
    }

    gfx_End();
    return 0;
}