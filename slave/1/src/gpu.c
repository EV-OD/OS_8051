#include "gpu.h"
#include "graphics.h"

#define MAX_BITMAP_SIZE 1024
__xdata unsigned char bitmap_buffer[MAX_BITMAP_SIZE];


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
    
    while(1) {
        cmd = gpu_receive_byte(); // Get Command ID
        unsigned char x, y, w, h;
        unsigned int n_bytes, i;
        
        switch(cmd) {
            case CMD_LINE:
                // Direct calls to receivers for parameters
                gfx_line(gpu_receive_byte(), gpu_receive_byte(), 
                         gpu_receive_byte(), gpu_receive_byte());
                break;
                
            case CMD_CIRCLE:
                gfx_circle(gpu_receive_byte(), gpu_receive_byte(), 
                           gpu_receive_byte());
                break;
            case CMD_RECT:
                gfx_rect(gpu_receive_byte(), gpu_receive_byte(),
                         gpu_receive_byte(), gpu_receive_byte());
                break;
                
            case CMD_BITMAP:
                P3_0 = 0; // Debug: Set pin low at start of bitmap command
                x = gpu_receive_byte();
                y = gpu_receive_byte();
                w = gpu_receive_byte();
                h = gpu_receive_byte();
                n_bytes = ((unsigned int)(w + 7u) / 8u) * (unsigned int)h;
                if (n_bytes > MAX_BITMAP_SIZE) n_bytes = MAX_BITMAP_SIZE;
                for (i = 0; i < n_bytes; i++) {
                    if (i < MAX_BITMAP_SIZE) {
                        bitmap_buffer[i] = gpu_receive_byte();
                    }
                }
                P3_0 = 1; // Debug: Set pin high after receiving bitmap data
                
                gfx_bitmap(x, y, w, h, bitmap_buffer);
                break;
                
            case CMD_CLS:
                lcd_clear_all();
                break;
        }
    }
}