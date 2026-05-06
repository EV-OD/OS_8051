#ifndef GPU_CONTROLLER_H
#define GPU_CONTROLLER_H

#include <reg51.h>

#define STB P3_2
#define BUSY P3_3

#define GPU_DATA P1      // Data Bus

/* Instruction Set IDs */
#define CMD_CLS      0x01
#define CMD_LINE     0x02
#define CMD_CIRCLE   0x03
#define CMD_RECT     0x04
#define CMD_BITMAP   0x05
#define CMD_PIXEL    0x06
#define CMD_FIXED    0x07

/**
 * Internal helper to send a single byte with a hardware handshake.
 */
void gpu_write(unsigned char val) {
    // 1. Wait until the GPU is ready to receive a NEW byte
    while (BUSY == 1); 

    // 2. Place data on the bus
    GPU_DATA = val;

    // 3. Signal to the Slave that data is stable and ready
    STB = 0;

    // 4. Wait for the Slave to pull BUSY high, acknowledging it caught the byte
    while (BUSY == 0);

    // 5. Release the strobe
    STB = 1;
}


/* Clear Screen */
void gpu_cls() {
    gpu_write(CMD_CLS);
}

/* Draw Line: x1, y1, x2, y2 */
void gpu_draw_line(unsigned char x1, unsigned char y1, 
                   unsigned char x2, unsigned char y2) {
    gpu_write(CMD_LINE);
    gpu_write(x1);
    gpu_write(y1);
    gpu_write(x2);
    gpu_write(y2);
}

/* Draw Circle: x, y, radius */
void gpu_draw_circle(unsigned char x, unsigned char y, unsigned char r) {
    gpu_write(CMD_CIRCLE);
    gpu_write(x);
    gpu_write(y);
    gpu_write(r);
}

/* Draw Rectangle: x, y, width, height */
void gpu_draw_rect(unsigned char x, unsigned char y, 
                   unsigned char w, unsigned char h) {
    gpu_write(CMD_RECT);
    gpu_write(x);
    gpu_write(y);
    gpu_write(w);
    gpu_write(h);
}

/* Set Pixel: x, y, state (1=Set, 0=Clear) */
void gpu_set_pixel(unsigned char x, unsigned char y, unsigned char state) {
    gpu_write(CMD_PIXEL);
    gpu_write(x);
    gpu_write(y);
    gpu_write(state);
}

void gpu_draw_fixed(){
    gpu_write(CMD_FIXED);
}



/**
 * Send a bitmap to the GPU.
 * @param data: Pointer to the bitmap array in Master's memory.
 */
void gpu_draw_bitmap(unsigned char x, unsigned char y, 
                     unsigned char w, unsigned char h, 
                     const __code unsigned char *bmp_data) {
    unsigned int i;

    // Calculate how many bytes to send based on dimensions
    unsigned int total_bytes = ((unsigned int)(w + 7u) / 8u) * (unsigned int)h;

    gpu_write(CMD_BITMAP);
    gpu_write(x);
    gpu_write(y);
    gpu_write(w);
    gpu_write(h);

    // Send the raw pixel data byte-by-byte
    for(i = 0; i < total_bytes; i++) {
        gpu_write(bmp_data[i]);
    }
}

#endif /* GPU_CONTROLLER_H */