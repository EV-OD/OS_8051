#include "firmware.h"
#include "gpu_controller.h"

void main(void)
{
    m_entry();
    STB = 1;
    for (;;)
    {
        gpu_cls();

        //other work
        gpu_draw_line(0, 0, 239, 127);
        gpu_draw_circle(120, 64, 30);
        gpu_draw_rect(50, 50, 100, 100);
        gpu_draw_line(0, 127, 239, 0);

        gpu_draw_fixed();
        
        for (unsigned char i = 0; i < 100; i++) {
            gpu_set_pixel(10 + i, 10 + i, 1);
        }



        
        // 8051 Bitmap - 15x15 pixels
        // Orientation: Horizontal, MSB-first
        // Bytes per row: 2 | Total bytes: 30
        // const __code unsigned char bitmap[] = {
        //     0x00, 0x00, 0x7F, 0xFC, 0x7F, 0xFC, 0x7F, 0xFC, 0x7F, 0xFC, 0x00, 0x3C,
        //     0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C,
        //     0x00, 0x3C, 0x00, 0x3C, 0x00, 0x00
        // };

        
        
        // gpu_draw_bitmap(10, 10, 15, 15, bitmap);
        
    }
}
