#ifndef GPU_CONTROLLER_H
#define GPU_CONTROLLER_H

#include <reg51.h>

#define STB P3_4
#define BUSY P3_5

#define GPU_DATA P1      // Data Bus

/* Instruction Set IDs */
#define CMD_CLS      0x01
#define CMD_LINE     0x02
#define CMD_CIRCLE   0x03
#define CMD_RECT     0x04
#define CMD_BITMAP   0x05
#define CMD_PIXEL    0x06
#define CMD_FIXED    0x07

// Extended rectangle commands (x1, y1, x2, y2)
#define CMD_FILL_RECT   0x08
#define CMD_CLEAR_RECT  0x09
#define CMD_INVERT_RECT 0x0A

// Page flipping / double-buffer control
#define CMD_SET_DRAW_PAGE    0x0B
#define CMD_SET_DISPLAY_PAGE 0x0C
#define CMD_SWAP_PAGES       0x0D
#define CMD_CLEAR_DRAW_PAGE  0x0E

// Text rendering (writes into LCD text plane)
#define CMD_TEXT             0x0F

/**
 * Internal helper to send a single byte with a hardware handshake.
 */
void gpu_write(unsigned char val) __reentrant {
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
void gpu_cls() __reentrant {
    gpu_write(CMD_CLS);
}

/* Draw Line: x1, y1, x2, y2 */
void gpu_draw_line(unsigned char x1, unsigned char y1, 
                   unsigned char x2, unsigned char y2) __reentrant {
    gpu_write(CMD_LINE);
    gpu_write(x1);
    gpu_write(y1);
    gpu_write(x2);
    gpu_write(y2);
}

/* Draw Circle: x, y, radius */
void gpu_draw_circle(unsigned char x, unsigned char y, unsigned char r) __reentrant {
    gpu_write(CMD_CIRCLE);
    gpu_write(x);
    gpu_write(y);
    gpu_write(r);
}

/*
 * Rectangle note:
 * The slave-side `gfx_rect()` expects (x1,y1,x2,y2) corners (inclusive),
 * not (x,y,width,height). These helpers accept width/height and convert.
 */
static void gpu_rect_send_xyxy(unsigned char cmd,
                              unsigned char x1, unsigned char y1,
                              unsigned char x2, unsigned char y2)
                              __reentrant
{
    gpu_write(cmd);
    gpu_write(x1);
    gpu_write(y1);
    gpu_write(x2);
    gpu_write(y2);
}

static void gpu_rect_send_wh(unsigned char cmd,
                            unsigned char x, unsigned char y,
                            unsigned char w, unsigned char h)
                            __reentrant
{
    unsigned int x2u;
    unsigned int y2u;
    unsigned char x2;
    unsigned char y2;

    if (w == 0u || h == 0u) {
        return;
    }

    x2u = (unsigned int)x + (unsigned int)w - 1u;
    y2u = (unsigned int)y + (unsigned int)h - 1u;

    x2 = (x2u >= 240u) ? 239u : (unsigned char)x2u;
    y2 = (y2u >= 128u) ? 127u : (unsigned char)y2u;

    gpu_rect_send_xyxy(cmd, x, y, x2, y2);
}

/* Draw hollow rectangle (outline): x, y, width, height */
void gpu_draw_rect(unsigned char x, unsigned char y,
                   unsigned char w, unsigned char h) __reentrant {
    gpu_rect_send_wh(CMD_RECT, x, y, w, h);
}

/* Draw solid filled rectangle: x, y, width, height */
void gpu_fill_rect(unsigned char x, unsigned char y,
                   unsigned char w, unsigned char h) __reentrant {
    gpu_rect_send_wh(CMD_FILL_RECT, x, y, w, h);
}

/* Clear (erase) a rectangle region: x, y, width, height */
void gpu_clear_rect(unsigned char x, unsigned char y,
                    unsigned char w, unsigned char h) __reentrant {
    gpu_rect_send_wh(CMD_CLEAR_RECT, x, y, w, h);
}

/* Invert a rectangle region: x, y, width, height */
void gpu_invert_rect(unsigned char x, unsigned char y,
                     unsigned char w, unsigned char h) __reentrant {
    gpu_rect_send_wh(CMD_INVERT_RECT, x, y, w, h);
}

/* Select which graphic page subsequent drawing commands write to. */
void gpu_set_draw_page(unsigned char page)
    __reentrant
{
    gpu_write(CMD_SET_DRAW_PAGE);
    gpu_write(page);
}

/* Select which graphic page is currently displayed. */
void gpu_set_display_page(unsigned char page)
    __reentrant
{
    gpu_write(CMD_SET_DISPLAY_PAGE);
    gpu_write(page);
}

/* Atomically swap display and draw pages (toggles 0<->1). */
void gpu_swap_pages(void)
    __reentrant
{
    gpu_write(CMD_SWAP_PAGES);
}

/* Clear the current draw page (entire graphic plane). */
void gpu_clear_draw_page(void)
    __reentrant
{
    gpu_write(CMD_CLEAR_DRAW_PAGE);
}

/* Set Pixel: x, y, state (1=Set, 0=Clear) */
void gpu_set_pixel(unsigned char x, unsigned char y, unsigned char state) __reentrant {
    gpu_write(CMD_PIXEL);
    gpu_write(x);
    gpu_write(y);
    gpu_write(state);
}

void gpu_draw_fixed(void) __reentrant {
    gpu_write(CMD_FIXED);
}

/*
 * Text sending helpers (macros) — keeps master internal RAM (DSEG) tiny.
 *
 * Protocol:
 *   CMD_TEXT, col, row, 'H','i',..., 0x00
 */
#define gpu_text(col, row, str_ptr) do {             \
    const unsigned char * _s = (str_ptr);            \
    gpu_write(CMD_TEXT);                             \
    gpu_write((unsigned char)(col));                 \
    gpu_write((unsigned char)(row));                 \
    if (_s) {                                        \
        while (*_s) gpu_write(*_s++);                \
    }                                                \
    gpu_write(0x00u);                                \
} while (0)

/* Pixel coordinate convenience for default FONT_8x8 (x/8,y/8). */
#define gpu_text_xy(x, y, str_ptr) \
    gpu_text((unsigned char)((x) >> 3), (unsigned char)((y) >> 3), (str_ptr))



/**
 * Send a bitmap to the GPU.
 * @param data: Pointer to the bitmap array in Master's memory.
 */
void gpu_draw_bitmap(unsigned char x, unsigned char y, 
                     unsigned char w, unsigned char h, 
                     const __code unsigned char *bmp_data) __reentrant {
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