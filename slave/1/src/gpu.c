#include "gpu.h"
#include "graphics.h"

#define MAX_BITMAP_SIZE 64
__idata unsigned char bitmap_buffer[] = {
    0x00, 0x00, 0x7F, 0xFC, 0x7F, 0xFC, 0x7F, 0xFC, 0x7F, 0xFC, 0x00, 0x3C,
    0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3C,
    0x00, 0x3C, 0x00, 0x3C, 0x00, 0x00
};

unsigned char gpu_receive_byte() {
    unsigned char val;
    
    BUSY = 0;           // Signal Master: "I am ready for the next byte"
    while (STB == 1);   // Wait for Master to drop STB (Master has placed data on P1)
    
    val = GPU_DATA;     // Capture the data from the bus
    BUSY = 1;           // Signal Master: "I've got it, I'm processing/busy"
    
    // Safety: Wait for Master to release STB before allowing the next read
    while (STB == 0); 
    
    return val;
}


void gpu_process_commands() {
    unsigned char cmd;
    
    unsigned char x, y, w, h;
    unsigned int n_bytes, i;
    while(1) {
        cmd = gpu_receive_byte(); // Get Command ID
        
        switch(cmd) {
            case CMD_LINE:
            // Direct calls to receivers for parameters
            x = gpu_receive_byte();
            y = gpu_receive_byte();
            w = gpu_receive_byte();
            h = gpu_receive_byte();
            gfx_line(x, y, w, h);
            break;
            
            case CMD_CIRCLE:
                gfx_circle(gpu_receive_byte(), gpu_receive_byte(), 
                           gpu_receive_byte());
                break;

            case CMD_RECT:
                gfx_rect(gpu_receive_byte(), gpu_receive_byte(),
                         gpu_receive_byte(), gpu_receive_byte());
                break;

            case CMD_FILL_RECT:
                gfx_fill_rect(gpu_receive_byte(), gpu_receive_byte(),
                              gpu_receive_byte(), gpu_receive_byte());
                break;

            case CMD_CLEAR_RECT:
                gfx_clear_rect(gpu_receive_byte(), gpu_receive_byte(),
                               gpu_receive_byte(), gpu_receive_byte());
                break;

            case CMD_INVERT_RECT:
                gfx_invert_rect(gpu_receive_byte(), gpu_receive_byte(),
                                gpu_receive_byte(), gpu_receive_byte());
                break;

            case CMD_PIXEL:
                x = gpu_receive_byte();
                y = gpu_receive_byte();
                w = gpu_receive_byte();
                gfx_pixel(x, y, w);
                break;
            
            case CMD_FIXED:
                gfx_bitmap_idata(10, 10, 15, 15, bitmap_buffer);
                break;
                
            case CMD_BITMAP:
                x = gpu_receive_byte();
                y = gpu_receive_byte();
                w = gpu_receive_byte();
                h = gpu_receive_byte();

                // Use 16-bit math for the count
                n_bytes = ((unsigned int)w + 7u) / 8u * (unsigned int)h;

                P3_0 = 0; // Debug: Start receiving bytes
                for (i = 0; i < n_bytes; i++) {
                    unsigned char b = gpu_receive_byte();
                    if (i < MAX_BITMAP_SIZE) {
                        bitmap_buffer[i] = b;
                    }
                }
                P3_0 = 1; // Debug: Receiving finished

                // Draw using the RAM buffer
                gfx_bitmap_idata(x, y, w, h, bitmap_buffer); 
                break;
                
            case CMD_CLS:
                lcd_clear_all();
                break;
        }
    }
}