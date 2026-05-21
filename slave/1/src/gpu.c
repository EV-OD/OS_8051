#include "gpu.h"
#include "graphics.h"

static __idata unsigned char display_page = 0u;
static __idata unsigned char draw_page = 0u;

static const unsigned char __code fixed_bitmap[] = {
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
    __idata unsigned char cmd;

    /* Ensure defaults are consistent */
    display_page = 0u;
    draw_page = 0u;
    lcd_set_graphic_page(display_page);
    gfx_set_draw_page(draw_page);
    
    __idata unsigned char x, y, w, h;
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

            case CMD_SET_DRAW_PAGE:
                draw_page = (unsigned char)(gpu_receive_byte() & 1u);
                gfx_set_draw_page(draw_page);
                break;

            case CMD_SET_DISPLAY_PAGE:
                display_page = (unsigned char)(gpu_receive_byte() & 1u);
                lcd_set_graphic_page(display_page);
                break;

            case CMD_SWAP_PAGES:
                display_page ^= 1u;
                draw_page ^= 1u;
                lcd_set_graphic_page(display_page);
                gfx_set_draw_page(draw_page);
                break;

            case CMD_CLEAR_DRAW_PAGE:
                lcd_clear_graphic_page(draw_page);
                break;

            case CMD_TEXT:
                /* Stream a null-terminated ASCII string and write into TEXT RAM.
                 * Params: col, row, bytes..., 0x00 terminator.
                 * No buffering to keep internal RAM usage tiny.
                 */
                {
                    __idata unsigned char col;
                    __idata unsigned char row;
                    __idata unsigned char ch;
                    __idata unsigned int addr;

                    col = gpu_receive_byte();
                    row = gpu_receive_byte();

                    /* If out of bounds, just drain the stream. */
                    if (col >= COLS || row >= ROWS)
                    {
                        do { ch = gpu_receive_byte(); } while (ch != 0x00u);
                        break;
                    }

                    addr = TXT_HOME
                         + (unsigned int)row * (unsigned int)COLS
                         + (unsigned int)col;

                    lcd_set_addr(addr);
                    lcd_write_cmd(CMD_AUTO_WRITE);

                    while (1)
                    {
                        ch = gpu_receive_byte();
                        if (ch == 0x00u) break;

                        /* Basic control handling */
                        if (ch == '\r')
                            continue;
                        if (ch == '\n')
                        {
                            col = 0u;
                            row++;
                            if (row >= ROWS) break;
                            lcd_auto_reset();
                            addr = TXT_HOME + (unsigned int)row * (unsigned int)COLS;
                            lcd_set_addr(addr);
                            lcd_write_cmd(CMD_AUTO_WRITE);
                            continue;
                        }

                        /* Match lcd_puts() mapping for internal CGROM */
                        if (ch >= 0x20u) ch = (unsigned char)(ch - 0x20u);

                        lcd_auto_write(ch);
                        col++;
                        if (col >= COLS)
                        {
                            col = 0u;
                            row++;
                            if (row >= ROWS) break;

                            lcd_auto_reset();
                            addr = TXT_HOME + (unsigned int)row * (unsigned int)COLS;
                            lcd_set_addr(addr);
                            lcd_write_cmd(CMD_AUTO_WRITE);
                        }
                    }

                    lcd_auto_reset();
                }
                break;
            
            case CMD_FIXED:
                /* Draw the built-in 15x15 test sprite (2 bytes/row). */
                {
                    __idata unsigned char row;
                    __idata unsigned char byte_i;
                    __idata unsigned char bit_i;
                    __idata unsigned char col;
                    __idata unsigned char yy;
                    __idata unsigned char xx;
                    __idata unsigned char b;
                    __idata unsigned char idx;

                    for (row = 0u; row < 15u; row++)
                    {
                        yy = (unsigned char)(10u + row);
                        if (yy < 10u || yy >= LCD_PIXEL_H) continue;

                        for (byte_i = 0u; byte_i < 2u; byte_i++)
                        {
                            idx = (unsigned char)((row << 1) + byte_i);
                            b = fixed_bitmap[idx];

                            for (bit_i = 0u; bit_i < 8u; bit_i++)
                            {
                                col = (unsigned char)((byte_i << 3) + bit_i);
                                if (col >= 15u) break;

                                xx = (unsigned char)(10u + col);
                                if (xx < 10u || xx >= LCD_PIXEL_W) continue;

                                gfx_pixel(
                                    xx,
                                    yy,
                                    (b & (unsigned char)(0x80u >> bit_i)) ? PIXEL_SET : PIXEL_CLEAR
                                );
                            }
                        }
                    }
                }
                break;
                
            case CMD_BITMAP:
                x = gpu_receive_byte();
                y = gpu_receive_byte();
                w = gpu_receive_byte();
                h = gpu_receive_byte();

                /* Stream bitmap bytes directly (no RAM buffering).
                 * Bitmap format: packed 1bpp, MSB=leftmost pixel.
                 */
                {
                    __idata unsigned char row;
                    __idata unsigned char byte_i;
                    __idata unsigned char bit_i;
                    __idata unsigned char bytes_per_row;
                    __idata unsigned char b;
                    __idata unsigned char col;
                    __idata unsigned char yy;
                    __idata unsigned char xx;

                    if (w == 0u || h == 0u) break;

                    /* ceil(w/8) without pulling in division helpers */
                    bytes_per_row = (unsigned char)((w + 7u) >> 3);

                    P3_0 = 0; // Debug: Start receiving bytes
                    for (row = 0u; row < h; row++)
                    {
                        for (byte_i = 0u; byte_i < bytes_per_row; byte_i++)
                        {
                            b = gpu_receive_byte();

                            /* If outside LCD bounds, still drain the stream. */
                            yy = (unsigned char)(y + row);
                            if (yy < y || yy >= LCD_PIXEL_H) continue;

                            for (bit_i = 0u; bit_i < 8u; bit_i++)
                            {
                                col = (unsigned char)((byte_i << 3) + bit_i);
                                if (col >= w) break;
                                xx = (unsigned char)(x + col);
                                if (xx < x || xx >= LCD_PIXEL_W) continue;

                                gfx_pixel(
                                    xx,
                                    yy,
                                    (b & (unsigned char)(0x80u >> bit_i)) ? PIXEL_SET : PIXEL_CLEAR
                                );
                            }
                        }
                    }
                    P3_0 = 1; // Debug: Receiving finished
                }
                break;
                
            case CMD_CLS:
                lcd_clear_all();
                break;
        }
    }
}