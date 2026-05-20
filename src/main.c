#include "firmware.h"
#include "gpu_controller.h"
#include "keyboard_controller.h"


void main(void)
{
    m_entry();
    STB = 1;
    keyboard_init();

    __bit started = 0;
    __bit shouldGoToNext = 0;
    unsigned char nextcounter = 0;
    gpu_cls();
    
    for (;;)
    {
        KEY_STATE key = isKeyPressed();
        switch (key) {
            case START:
                if (!started) {
                    gpu_draw_rect(50, 50, 150, 150);
                    started = 1;
                }
                break;
            case NEXT:
                if (started){
                    if (nextcounter == 0){
                        gpu_cls();
                    }else if (nextcounter == 1){
                        gpu_draw_rect(50, 50, 100, 100);
                    }else if (nextcounter == 2){
                        gpu_draw_line(0, 127, 239, 0);
                    }else if (nextcounter == 3){
                        const __code unsigned char bitmap[] = {
                            0x00, 0x00, 0x7F, 0xFC, 0x7F, 0xFC, 0x7F,
                            0xFC, 0x7F, 0xFC, 0x00, 0x3C,
                            0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00,
                            0x3C, 0x00, 0x3C, 0x00, 0x3C,
                            0x00, 0x3C, 0x00, 0x3C, 0x00, 0x00
                        };
                        gpu_draw_bitmap(10, 10, 15, 15, bitmap);
                    }
                    else if (nextcounter == 4){
                        //    make home
                        gpu_draw_line(0, 127, 239, 0);
                        gpu_draw_line(0, 0, 239, 127);
                        gpu_draw_rect(50, 50, 150, 150);
                        
                    }
                    nextcounter = (nextcounter + 1) % 5;
                }
                break;
            default:
                gpu_draw_circle(120, 64, 30);
                break;
        }
        

        // gpu_draw_line(0, 0, 239, 127);
        // gpu_draw_rect(50, 50, 100, 100);
        // gpu_draw_line(0, 127, 239, 0);

        // gpu_draw_fixed();
    



        
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
