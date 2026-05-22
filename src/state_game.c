#include "common.h"
#include "state_menu.h"
#include <sys/timers.h>

#define TICKS_PER_FRAME (32768 / 30)
#define BLOCK_HEIGHT 20
#define INITIAL_SIZE 40
#define TOWER_MAX 6

typedef enum
{
    AXIS_X,
    AXIS_Y
} MoveAxis;

typedef struct
{
    int x;
    int y;
    int w;
    int l;
    uint8_t color_index;
} StackBlock;

static uint8_t hue_circle[] = {
    0xA0,
    0xC2,
    0xC4,
    0xA6,
    0x33,
    0x31,
    0x50};

static uint32_t last_time;

static uint32_t score;
static uint32_t high_score;

static StackBlock tower[TOWER_MAX];
static StackBlock active_block;
static int tower_count;
static int camera_y;

static MoveAxis current_axis;
static int error;
static int move_dir;

static bool exit_confirmation = false;
static bool game_over = false;
static uint8_t perfect_timer;

void saveHighScore(uint32_t current_score)
{
    uint8_t slot;
    slot = ti_Open(HIGH_SCORE_APPVAR_NAME, "w");
    if (slot)
    {
        ti_Write(&current_score, sizeof(uint32_t), 1, slot);
        ti_SetArchiveStatus(true, slot);
        ti_Close(slot);
    }
}

uint32_t loadHighScore(void)
{
    uint8_t slot;
    uint32_t high_score = 0;

    slot = ti_Open(HIGH_SCORE_APPVAR_NAME, "r");
    if (slot)
    {
        ti_Read(&high_score, sizeof(uint32_t), 1, slot);
        ti_Close(slot);
    }

    return high_score;
}

static void
drawIsometricBlock(int wx, int wy, int wz, int w, int l, int h, int ox, int oy, uint8_t color_top, uint8_t color_left, uint8_t color_right)
{
    int i, cur_x;

    int p2x = ox + 2 * wy - 2 * wx - 2 * w;
    int p2y = oy + wy + wx - wz + w;
    int p3x = p2x + 2 * l;
    int p3y = p2y + l;

    gfx_SetColor(color_left);
    cur_x = p2x;
    int side_y = p2y;
    for (i = 0; i < l; i++)
    {
        gfx_VertLine(cur_x, side_y, h);
        gfx_VertLine(cur_x + 1, side_y, h);
        cur_x += 2;
        side_y++;
    }

    gfx_SetColor(color_right);
    cur_x = p3x;
    side_y = p3y;
    for (i = 0; i < w; i++)
    {
        gfx_VertLine(cur_x, side_y, h);
        gfx_VertLine(cur_x + 1, side_y, h);
        cur_x += 2;
        side_y--;
    }

    gfx_SetColor(color_top);
    cur_x = p2x;
    int ty = p2y;
    int slice_h = 1;

    int limit1 = (w < l) ? w : l;
    int limit2 = (w > l) ? (w - l) : (l - w);

    for (i = 0; i < limit1; i++)
    {
        gfx_VertLine(cur_x, ty, slice_h);
        gfx_VertLine(cur_x + 1, ty, slice_h);
        cur_x += 2;
        ty--;
        slice_h += 2;
    }

    if (w > l)
    {
        for (i = 0; i < limit2; i++)
        {
            gfx_VertLine(cur_x, ty, slice_h);
            gfx_VertLine(cur_x + 1, ty, slice_h);
            cur_x += 2;
            ty--;
        }
    }
    else if (l > w)
    {
        for (i = 0; i < limit2; i++)
        {
            gfx_VertLine(cur_x, ty, slice_h);
            gfx_VertLine(cur_x + 1, ty, slice_h);
            cur_x += 2;
            ty++;
        }
    }

    for (i = 0; i < limit1; i++)
    {
        gfx_VertLine(cur_x, ty, slice_h);
        gfx_VertLine(cur_x + 1, ty, slice_h);
        cur_x += 2;
        ty++;
        slice_h -= 2;
    }
}

static void spawnNewBlock()
{
    StackBlock *base = &tower[tower_count - 1];
    current_axis = (current_axis == AXIS_X) ? AXIS_Y : AXIS_X;

    move_dir = 1;

    error = -INITIAL_SIZE - 10;

    active_block.x = base->x;
    active_block.y = base->y;
    active_block.w = base->w;
    active_block.l = base->l;
    active_block.color_index = (base->color_index + 1) % sizeof(hue_circle);
}

static void placeActiveBlock()
{
    StackBlock *base = &tower[tower_count - 1];

    int max_error = (current_axis == AXIS_X) ? base->w : base->l;
    if (abs(error) >= max_error)
    {
        game_over = true;
        if (score > high_score)
        {
            high_score = score;
            saveHighScore(high_score);
        }
        return;
    }

    if (abs(error) <= 2)
    {
        perfect_timer = 20;
        error = 0;
    }

    StackBlock placed;
    placed.color_index = active_block.color_index;

    if (current_axis == AXIS_X)
    {
        placed.w = base->w - abs(error);
        placed.l = base->l;
        placed.y = base->y;

        if (error > 0)
        {
            placed.x = base->x + error;
        }
        else
        {
            placed.x = base->x;
        }
    }
    else
    {
        placed.w = base->w;
        placed.l = base->l - abs(error);
        placed.x = base->x;

        if (error > 0)
        {
            placed.y = base->y + error;
        }
        else
        {
            placed.y = base->y;
        }
    }

    if (tower_count < TOWER_MAX)
    {
        tower[tower_count] = placed;
        tower_count++;
    }
    else
    {
        for (int i = 1; i < TOWER_MAX; i++)
        {
            tower[i - 1] = tower[i];
        }
        tower[TOWER_MAX - 1] = placed;
        camera_y -= BLOCK_HEIGHT;
    }

    score++;
    spawnNewBlock();
}

static void game_init(void)
{
    timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);
    last_time = timer_Get(1);

    score = 0;
    high_score = loadHighScore();
    exit_confirmation = false;
    game_over = false;
    perfect_timer = 0;

    current_axis = AXIS_X;
    camera_y = -BLOCK_HEIGHT * TOWER_MAX;

    tower[0].x = 0;
    tower[0].y = 0;
    tower[0].w = INITIAL_SIZE;
    tower[0].l = INITIAL_SIZE;
    tower[0].color_index = 0;
    tower_count = 1;

    spawnNewBlock();
}

static void game_step(void)
{
    uint32_t current_time = timer_Get(1);

    if (keyPressed(kb_KeyClear))
    {
        if (game_over)
            enterState(STATE_MENU);
        else
            exit_confirmation = !exit_confirmation;
        return;
    }

    if (keyPressed(kb_KeyEnter))
    {
        if (game_over)
            game_init();
        else if (exit_confirmation)
            enterState(STATE_MENU);
        else
            placeActiveBlock();
        return;
    }

    if (game_over || exit_confirmation)
        return;

    while (current_time - last_time < TICKS_PER_FRAME)
        current_time = timer_Get(1);
    last_time = current_time;

    error += move_dir * 2;
    if (abs(error) > INITIAL_SIZE + 10)
    {
        move_dir = -move_dir;
    }

    if (camera_y < -(TOWER_MAX - tower_count) * BLOCK_HEIGHT)
    {
        camera_y++;
    }

    if (perfect_timer > 0)
        perfect_timer--;
}

static void game_draw(void)
{
    uint8_t active_color_index = hue_circle[active_block.color_index];
    gfx_ZeroScreen();

    int screen_base_y = 240 + camera_y;

    // bottom blocks
    int current_z = 0;
    for (int i = 0; i < tower_count; i++)
    {
        StackBlock this = tower[i];
        uint8_t color_index = hue_circle[this.color_index];
        drawIsometricBlock(this.x, this.y, current_z, this.w, this.l, BLOCK_HEIGHT, GFX_LCD_WIDTH / 2, screen_base_y, color_index + 0x20, color_index, color_index - 0x20);

        current_z += BLOCK_HEIGHT;
    }

    // active block
    int wx = active_block.x;
    int wy = active_block.y;
    if (current_axis == AXIS_X)
    {
        wx += error;
    }
    else
    {
        wy += error;
    }
    drawIsometricBlock(wx, wy, current_z, active_block.w, active_block.l, BLOCK_HEIGHT, GFX_LCD_WIDTH / 2, screen_base_y, active_color_index + 0x20, active_color_index, active_color_index - 0x20);

    gfx_SetTextFGColor(active_color_index);
    gfx_SetTextXY(10, 10);
    gfx_PrintString("SCORE: ");
    gfx_PrintUInt(score, 1);

    gfx_SetTextFGColor(0xFF);
    gfx_SetTextBGColor(0x00);
    if (perfect_timer > 0)
    {
        printCentered("PERFECT!", 120);
    }
    if (game_over)
    {
        printCentered("GAME OVER!", 100);
        printCentered("Press <clear> to exit", 115);
        printCentered("Press <enter> to try again", 130);
    }
    else if (exit_confirmation)
    {
        printCentered("Press <enter> to exit", 105);
        printCentered("Press <clear> to cancel", 120);
    }

    gfx_SwapDraw();
}

const GameState STATE_GAME = {game_init, game_step, game_draw};