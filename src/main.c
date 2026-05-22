#include <stdbool.h>
#include <graphx.h>
#include <keypadc.h>

bool step()
{
    kb_Scan();

    return 0;
}

void draw()
{

}

int main(void)
{
    gfx_Begin();

    while (step())
    {
        draw();
        gfx_SwapDraw();
    }
    

    gfx_End();
    return 0;
}