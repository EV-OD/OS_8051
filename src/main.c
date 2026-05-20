#include "firmware.h"
#include "gpu_controller.h"
#include "keyboard_controller.h"


#define SCREEN_W 240u
#define SCREEN_H 128u

#define RECT_W 6u
#define RECT_H 6u
#define STEP   5u

void main(void)
{
    m_entry();
    STB = 1;
    keyboard_init();

    {
        unsigned char x = 50u;
        unsigned char y = 50u;

        unsigned char draw_page = 1u;
        unsigned char page_x[2] = { 50u, 50u };
        unsigned char page_y[2] = { 50u, 50u };

        const unsigned char max_x = (unsigned char)(SCREEN_W - (unsigned char)RECT_W);
        const unsigned char max_y = (unsigned char)(SCREEN_H - (unsigned char)RECT_H);

        if (x > max_x) x = max_x;
        if (y > max_y) y = max_y;

        /* Init both pages with the same initial box */
        gpu_set_draw_page(0u);
        gpu_clear_draw_page();
        gpu_fill_rect(x, y, (unsigned char)RECT_W, (unsigned char)RECT_H);

        gpu_set_draw_page(1u);
        gpu_clear_draw_page();
        gpu_fill_rect(x, y, (unsigned char)RECT_W, (unsigned char)RECT_H);

        gpu_set_display_page(0u);
        gpu_set_draw_page(1u);

        for (;;)
        {
            __bit moved = 0;
            unsigned char old_x = page_x[draw_page];
            unsigned char old_y = page_y[draw_page];
            KEY_STATE key = isKeyPressed();

            switch (key) {
                case UP:
                    if (y >= (unsigned char)STEP) y = (unsigned char)(y - (unsigned char)STEP);
                    else y = 0u;
                    moved = 1;
                    break;
                case DOWN:
                    if (y <= (unsigned char)(max_y - (unsigned char)STEP)) y = (unsigned char)(y + (unsigned char)STEP);
                    else y = max_y;
                    moved = 1;
                    break;
                case LEFT:
                    if (x >= (unsigned char)STEP) x = (unsigned char)(x - (unsigned char)STEP);
                    else x = 0u;
                    moved = 1;
                    break;
                case RIGHT:
                    if (x <= (unsigned char)(max_x - (unsigned char)STEP)) x = (unsigned char)(x + (unsigned char)STEP);
                    else x = max_x;
                    moved = 1;
                    break;
                default:
                    break;
            }

            if (moved) {
                gpu_clear_rect(old_x, old_y, (unsigned char)RECT_W, (unsigned char)RECT_H);
                gpu_fill_rect(x, y, (unsigned char)RECT_W, (unsigned char)RECT_H);

                page_x[draw_page] = x;
                page_y[draw_page] = y;

                gpu_swap_pages();
                draw_page ^= 1u;
            }
        }
    }
}
