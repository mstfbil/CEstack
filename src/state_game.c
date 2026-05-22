#include "common.h"
#include "state_menu.h"

#define BLOCK_HEIGHT 20
#define INITIAL_SIZE 40
#define TOWER_MAX 5

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

static StackBlock tower[TOWER_MAX];
static StackBlock active_block;
static int tower_count;
static int camera_y;

static MoveAxis current_axis;
static int error;
static int move_dir;

static void
drawIsometricBlock(int wx, int wy, int wz, int w, int l, int h, int ox, int oy, uint8_t color_top, uint8_t color_left, uint8_t color_right)
{
    int p1x = ox + (2 * wy) - (2 * wx);
    int p1y = oy + wy + wx - wz;
    int p2x = p1x - 2 * w;
    int p2y = p1y + w;
    int p3x = p1x - 2 * w + 2 * l;
    int p3y = p1y + w + l;
    int p4x = p1x + 2 * l;
    int p4y = p1y + l;

    int i;

    gfx_SetColor(color_left);
    for (i = 0; i < 2 * l; i++)
    {
        gfx_VertLine(p2x + i, p2y + (i >> 1), h);
    }

    gfx_SetColor(color_right);
    for (i = 0; i < 2 * w; i++)
    {
        gfx_VertLine(p3x + i, p3y - (i >> 1), h);
    }

    gfx_SetColor(color_top);
    gfx_FillTriangle(p1x, p1y, p2x, p2y, p3x, p3y);
    gfx_FillTriangle(p1x, p1y, p3x, p3y, p4x, p4y);
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
        enterState(STATE_MENU);
        return;
    }

    if (abs(error) <= 2)
    {
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

    spawnNewBlock();
}

static void game_init(void)
{
    current_axis = AXIS_X;
    camera_y = 0;

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
    if (keyPressed(kb_KeyClear))
    {
        enterState(STATE_MENU);
        return;
    }

    if (keyPressed(kb_KeyEnter))
    {
        placeActiveBlock();
        return;
    }

    error += move_dir * 2;
    if (abs(error) > INITIAL_SIZE + 10)
    {
        move_dir = -move_dir;
    }

    if (camera_y < 0)
    {
        camera_y++;
    }
}

static void game_draw(void)
{
    gfx_FillScreen(0x00);

    int screen_base_y = 200 + camera_y;

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
    uint8_t color_index = hue_circle[active_block.color_index];
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
    drawIsometricBlock(wx, wy, current_z, active_block.w, active_block.l, BLOCK_HEIGHT, GFX_LCD_WIDTH / 2, screen_base_y, color_index + 0x20, color_index, color_index - 0x20);

    gfx_SwapDraw();
}

const GameState STATE_GAME = {game_init, game_step, game_draw};