#include "slave1.h"
#include "setup.h"
#include "graphics.h"
#include "gpu.h"

void main(void)
{
    s1_ent();
    lcd_init();                         /* hardware reset + configure  */


    for (;;)
    {
        // /* ── graphic demos ── */
        // gfx_rect(4, 4, 235, 123);          /* border rect                 */
        // // gfx_line(4, 4, 235, 123);          /* diagonal corner-to-corner   */
        // // gfx_line(235, 4, 4, 123);

        // const __code unsigned char x_sprite[] = { 0x88, 0x50, 0x20, 0x50, 0x88 };


        // const __idata unsigned char bitmap[] = {
        //     0x00, 0x00, 0x7F, 0xFC, 0x7F, 0xFC, 0x7F, 0xFC, 0x7F, 0xFC, 0x00, 0x3C,
        //     0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C,
        //     0x00, 0x3C, 0x00, 0x3C, 0x00, 0x00
        // };
        // gfx_bitmap_xdata(10, 10, 15, 15, bitmap); // Works perfectly!
        // gfx_bitmap(10, 10, 15, 15, bitmap);


        // Your RAM array call:
        // const __xdata unsigned char bitmap[] = { ... };

        // Your ROM Flash constant array call:
        // const __code unsigned char flash_bitmap[] = {
        //     0x00, 0x00, 0x7F, 0xFC, 0x7F, 0xFC, 0x7F, 0xFC, 0x7F, 0xFC, 0x00, 0x3C,
        //     0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C,
        //     0x00, 0x3C, 0x00, 0x3C, 0x00, 0x00
        // };
        // gfx_bitmap_code(10, 10, 15, 15, flash_bitmap); // Works perfectly!



        // gfx_circle(120, 64, 40);           /* centred hollow circle       */
        // gfx_fill_circle(120, 64, 15);      /* solid inner dot             */

        // gfx_fill_rect(10, 100, 60, 120);   /* solid block in corner       */
        // gfx_invert_rect(10, 100, 60, 120); /* invert it back              */

        // /* ── text overlay ── */
        // lcd_set_display_mode(CMD_DISP_BOTH_ON);
        // // lcd_puts(3, 0, (__code unsigned char *)"LM3229 READY");


        gpu_process_commands();
    }
}
