/*
 * graphics.c — LM3229 / T6963C pixel drawing implementation
 */

#include "graphics.h"

/* ═══════════════════════════════════════════════════════════════
 * INTERNAL HELPERS
 * ═══════════════════════════════════════════════════════════════ */

/*
 * clip_x / clip_y — return 1 if coordinate is in bounds.
 * Used as guards at the top of each public function.
 */
#define IN_BOUNDS_X(x)  ((x) < LCD_PIXEL_W)
#define IN_BOUNDS_Y(y)  ((y) < LCD_PIXEL_H)

/*
 * swap_uc — swap two unsigned char variables in-place.
 */
#define SWAP_UC(a, b)   do { unsigned char _t = (a); (a) = (b); (b) = _t; } while(0)

/*
 * abs_diff — unsigned absolute difference (avoids signed overflow).
 */
static unsigned char abs_diff(unsigned char a, unsigned char b)
{
    return (a >= b) ? (a - b) : (b - a);
}

/*
 * gr_write_byte_at(addr, byte)
 * Write a raw byte directly into graphic RAM at addr.
 * Does NOT check status before the auto_write — caller must be
 * inside an open auto-write session, or call lcd_set_addr first.
 */
static void gr_write_byte_at(unsigned int addr, unsigned char byte)
{
    lcd_set_addr(addr);
    lcd_write_data(byte);
    lcd_write_cmd(CMD_DATA_WRITE_NOP);   /* write, ADP unchanged */
}

/*
 * gr_read_byte_at(addr)
 * Read a raw byte from graphic RAM — needed for masked edge writes.
 */
static unsigned char gr_read_byte_at(unsigned int addr)
{
    lcd_set_addr(addr);
    return lcd_read_data();
}

/*
 * gr_rmw_byte(addr, set_mask, clear_mask)
 * Read-Modify-Write a graphic RAM byte:
 *   set bits in set_mask,  clear bits in clear_mask.
 * Used for partial-byte writes at the edges of hline/fill_rect.
 */
static void gr_rmw_byte(unsigned int   addr,
                        unsigned char  set_mask,
                        unsigned char  clear_mask)
{
    unsigned char val = gr_read_byte_at(addr);
    val |=  set_mask;
    val &= ~clear_mask;
    gr_write_byte_at(addr, val);
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_pixel
 * ═══════════════════════════════════════════════════════════════ */
void gfx_pixel(unsigned char x, unsigned char y, unsigned char state)
{
    unsigned int  addr;
    unsigned char bit;

    if (!IN_BOUNDS_X(x) || !IN_BOUNDS_Y(y)) return;

    addr = GR_BYTE_ADDR(x, y);
    bit  = GR_BIT_POS(x);

    lcd_set_addr(addr);
    if (state)
        lcd_write_cmd(CMD_BIT_SET   | bit);
    else
        lcd_write_cmd(CMD_BIT_RESET | bit);
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_hline
 *
 * Strategy for a horizontal run from x1 to x2 on row y:
 *
 *   ┌──────┬────────────────────────┬──────┐
 *   │ left │    full middle bytes   │right │
 *   │ edge │   (write 0xFF each)    │ edge │
 *   └──────┴────────────────────────┴──────┘
 *
 * Left  edge: bits  [x1%8 .. 7]  within its byte
 * Right edge: bits  [0    .. x2%8] within its byte
 * If left and right fall in the same byte, merge the masks.
 * ═══════════════════════════════════════════════════════════════ */
void gfx_hline(unsigned char x1, unsigned char x2, unsigned char y)
{
    unsigned int  addr_l, addr_r, addr;
    unsigned char mask_l, mask_r, bit_l, bit_r;

    if (!IN_BOUNDS_Y(y)) return;
    if (x1 > x2) SWAP_UC(x1, x2);
    if (!IN_BOUNDS_X(x1)) return;
    if (x2 >= LCD_PIXEL_W) x2 = LCD_PIXEL_W - 1;

    addr_l = GR_BYTE_ADDR(x1, y);
    addr_r = GR_BYTE_ADDR(x2, y);

    bit_l  = GR_BIT_POS(x1);     /* leftmost bit to SET in left byte  */
    bit_r  = GR_BIT_POS(x2);     /* rightmost bit to SET in right byte*/

    /* Mask covering bits bit_l..7  (bits left of x1 within its byte) */
    /* Example: bit_l=5 → mask = 0b00111111 = 0x3F                    */
    mask_l = (unsigned char)((1u << (bit_l + 1u)) - 1u);

    /* Mask covering bits 0..bit_r */
    /* Example: bit_r=2 → mask = 0b11111100 = 0xFC                    */
    mask_r = (unsigned char)(0xFFu << bit_r);

    if (addr_l == addr_r)
    {
        /* Same byte — intersect the two masks */
        gr_rmw_byte(addr_l, mask_l & mask_r, 0x00u);
        return;
    }

    /* Left partial byte */
    gr_rmw_byte(addr_l, mask_l, 0x00u);

    /* Full middle bytes — use auto-write for speed */
    if (addr_r > addr_l + 1u)
    {
        lcd_set_addr(addr_l + 1u);
        lcd_write_cmd(CMD_AUTO_WRITE);
        for (addr = addr_l + 1u; addr < addr_r; addr++)
            lcd_auto_write(0xFFu);
        lcd_auto_reset();
    }

    /* Right partial byte */
    gr_rmw_byte(addr_r, mask_r, 0x00u);
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_vline
 * ═══════════════════════════════════════════════════════════════ */
void gfx_vline(unsigned char x, unsigned char y1, unsigned char y2)
{
    unsigned char y;
    unsigned int  addr;
    unsigned char bit;

    if (!IN_BOUNDS_X(x)) return;
    if (y1 > y2) SWAP_UC(y1, y2);
    if (!IN_BOUNDS_Y(y1)) return;
    if (y2 >= LCD_PIXEL_H) y2 = LCD_PIXEL_H - 1;

    bit = GR_BIT_POS(x);

    for (y = y1; y <= y2; y++)
    {
        addr = GR_BYTE_ADDR(x, y);
        lcd_set_addr(addr);
        lcd_write_cmd(CMD_BIT_SET | bit);
    }
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_line  — Bresenham integer line algorithm
 * ═══════════════════════════════════════════════════════════════ */
void gfx_line(unsigned char x1, unsigned char y1,
              unsigned char x2, unsigned char y2)
{
    // Use int for everything involved in the error calculation
    int dx, dy, err, e2;
    signed char sx, sy;

    // Calculate absolute distances
    if (x1 < x2) { dx = (int)(x2 - x1); sx = 1; }
    else         { dx = (int)(x1 - x2); sx = -1; }

    if (y1 < y2) { dy = (int)(y2 - y1); sy = 1; }
    else         { dy = (int)(y1 - y2); sy = -1; }

    // err = dx - dy
    err = dx - dy;

    while (1)
    {
        gfx_pixel(x1, y1, PIXEL_SET);

        // Exit condition
        if (x1 == x2 && y1 == y2) break;

        e2 = err << 1; // 2 * err

        // Vertical step check
        // We compare e2 with -dy. Since dy is positive, -dy is negative.
        if (e2 > -dy) 
        { 
            err -= dy; 
            x1 = (unsigned char)(x1 + sx); 
        }

        // Horizontal step check
        if (e2 < dx) 
        { 
            err += dx; 
            y1 = (unsigned char)(y1 + sy); 
        }
    }
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_rect
 * ═══════════════════════════════════════════════════════════════ */
void gfx_rect(unsigned char x1, unsigned char y1,
              unsigned char x2, unsigned char y2)
{
    if (x1 > x2) SWAP_UC(x1, x2);
    if (y1 > y2) SWAP_UC(y1, y2);

    gfx_hline(x1, x2, y1);   /* top    */
    gfx_hline(x1, x2, y2);   /* bottom */
    gfx_vline(x1, y1, y2);   /* left   */
    gfx_vline(x2, y1, y2);   /* right  */
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_fill_rect
 *
 * For each row in [y1..y2], call gfx_hline.
 * gfx_hline already handles partial-byte edges, so this is
 * correct for any alignment — no special cases needed here.
 * ═══════════════════════════════════════════════════════════════ */
void gfx_fill_rect(unsigned char x1, unsigned char y1,
                   unsigned char x2, unsigned char y2)
{
    unsigned char y;

    if (x1 > x2) SWAP_UC(x1, x2);
    if (y1 > y2) SWAP_UC(y1, y2);
    if (y2 >= LCD_PIXEL_H) y2 = LCD_PIXEL_H - 1;

    for (y = y1; y <= y2; y++)
        gfx_hline(x1, x2, y);
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_circle  — Bresenham midpoint circle algorithm
 *
 * Plots 8 symmetric octant points per step.
 * ═══════════════════════════════════════════════════════════════ */

/* Helper: plot the 8 octant points of circle (cx,cy) at offset (ox,oy) */
static void plot8(unsigned char cx, unsigned char cy,
                  unsigned char ox, unsigned char oy)
{
    /* Signed temporaries to detect underflow when cx < ox etc. */
    signed int x = (signed int)cx;
    signed int y = (signed int)cy;
    signed int a = (signed int)ox;
    signed int b = (signed int)oy;

    if (x+a < LCD_PIXEL_W && y+b < LCD_PIXEL_H) gfx_pixel((unsigned char)(x+a),(unsigned char)(y+b),PIXEL_SET);
    if (x-a >= 0 && x-a < LCD_PIXEL_W && y+b < LCD_PIXEL_H) gfx_pixel((unsigned char)(x-a),(unsigned char)(y+b),PIXEL_SET);
    if (x+a < LCD_PIXEL_W && y-b >= 0 && y-b < LCD_PIXEL_H) gfx_pixel((unsigned char)(x+a),(unsigned char)(y-b),PIXEL_SET);
    if (x-a >= 0 && x-a < LCD_PIXEL_W && y-b >= 0 && y-b < LCD_PIXEL_H) gfx_pixel((unsigned char)(x-a),(unsigned char)(y-b),PIXEL_SET);
    if (x+b < LCD_PIXEL_W && y+a < LCD_PIXEL_H) gfx_pixel((unsigned char)(x+b),(unsigned char)(y+a),PIXEL_SET);
    if (x-b >= 0 && x-b < LCD_PIXEL_W && y+a < LCD_PIXEL_H) gfx_pixel((unsigned char)(x-b),(unsigned char)(y+a),PIXEL_SET);
    if (x+b < LCD_PIXEL_W && y-a >= 0 && y-a < LCD_PIXEL_H) gfx_pixel((unsigned char)(x+b),(unsigned char)(y-a),PIXEL_SET);
    if (x-b >= 0 && x-b < LCD_PIXEL_W && y-a >= 0 && y-a < LCD_PIXEL_H) gfx_pixel((unsigned char)(x-b),(unsigned char)(y-a),PIXEL_SET);
}

void gfx_circle(unsigned char cx, unsigned char cy, unsigned char r)
{
    signed int x = 0;
    signed int y = (signed int)r;
    signed int d = 1 - (signed int)r;

    plot8(cx, cy, (unsigned char)x, (unsigned char)y);

    while (x < y)
    {
        x++;
        if (d < 0)
            d += 2 * x + 1;
        else
        {
            y--;
            d += 2 * (x - y) + 1;
        }
        plot8(cx, cy, (unsigned char)x, (unsigned char)y);
    }
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_fill_circle
 *
 * For each horizontal scanline that intersects the circle,
 * compute the chord endpoints and call gfx_hline.
 * Uses the same Bresenham iteration as gfx_circle.
 * ═══════════════════════════════════════════════════════════════ */
void gfx_fill_circle(unsigned char cx, unsigned char cy, unsigned char r)
{
    signed int x = 0;
    signed int y = (signed int)r;
    signed int d = 1 - (signed int)r;
    signed int c = (signed int)cx;
    signed int t = (signed int)cy;

    /* Helper lambda as macro — draw horizontal chord at row (t+oy)
     * and at (t-oy) from (c-ox) to (c+ox)                        */
    #define FILL_ROW(ox, oy)  do {                                      \
        signed int lx = c - (ox), rx = c + (ox);                        \
        if (lx < 0) lx = 0;                                             \
        if (rx >= LCD_PIXEL_W) rx = LCD_PIXEL_W - 1;                    \
        if (t+(oy) >= 0 && t+(oy) < LCD_PIXEL_H)                        \
            gfx_hline((unsigned char)lx,(unsigned char)rx,              \
                      (unsigned char)(t+(oy)));                          \
        if ((oy) != 0 && t-(oy) >= 0 && t-(oy) < LCD_PIXEL_H)           \
            gfx_hline((unsigned char)lx,(unsigned char)rx,              \
                      (unsigned char)(t-(oy)));                          \
    } while(0)

    FILL_ROW(x, y);

    while (x < y)
    {
        x++;
        if (d < 0)
            d += 2 * x + 1;
        else
        {
            y--;
            d += 2 * (x - y) + 1;
        }
        FILL_ROW(x, y);
        FILL_ROW(y, x);
    }

    #undef FILL_ROW
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_clear_rect
 * ═══════════════════════════════════════════════════════════════ */
void gfx_clear_rect(unsigned char x1, unsigned char y1,
                    unsigned char x2, unsigned char y2)
{
    unsigned char y;
    unsigned int  addr_l, addr_r, addr;
    unsigned char mask_l, mask_r, bit_l, bit_r;

    if (x1 > x2) SWAP_UC(x1, x2);
    if (y1 > y2) SWAP_UC(y1, y2);
    if (x2 >= LCD_PIXEL_W) x2 = LCD_PIXEL_W - 1;
    if (y2 >= LCD_PIXEL_H) y2 = LCD_PIXEL_H - 1;

    bit_l  = GR_BIT_POS(x1);
    bit_r  = GR_BIT_POS(x2);
    mask_l = (unsigned char)((1u << (bit_l + 1u)) - 1u);
    mask_r = (unsigned char)(0xFFu << bit_r);

    addr_l = GR_BYTE_ADDR(x1, y1);
    addr_r = GR_BYTE_ADDR(x2, y1);

    for (y = y1; y <= y2; y++)
    {
        addr_l = GR_BYTE_ADDR(x1, y);
        addr_r = GR_BYTE_ADDR(x2, y);

        if (addr_l == addr_r)
        {
            gr_rmw_byte(addr_l, 0x00u, mask_l & mask_r);
            continue;
        }

        gr_rmw_byte(addr_l, 0x00u, mask_l);

        if (addr_r > addr_l + 1u)
        {
            lcd_set_addr(addr_l + 1u);
            lcd_write_cmd(CMD_AUTO_WRITE);
            for (addr = addr_l + 1u; addr < addr_r; addr++)
                lcd_auto_write(0x00u);
            lcd_auto_reset();
        }

        gr_rmw_byte(addr_r, 0x00u, mask_r);
    }
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_invert_rect
 * ═══════════════════════════════════════════════════════════════ */
void gfx_invert_rect(unsigned char x1, unsigned char y1,
                     unsigned char x2, unsigned char y2)
{
    unsigned char y, val;
    unsigned int  addr;

    if (x1 > x2) SWAP_UC(x1, x2);
    if (y1 > y2) SWAP_UC(y1, y2);
    if (x2 >= LCD_PIXEL_W) x2 = LCD_PIXEL_W - 1;
    if (y2 >= LCD_PIXEL_H) y2 = LCD_PIXEL_H - 1;

    for (y = y1; y <= y2; y++)
    {
        unsigned int a_l = GR_BYTE_ADDR(x1, y);
        unsigned int a_r = GR_BYTE_ADDR(x2, y);

        for (addr = a_l; addr <= a_r; addr++)
        {
            val = gr_read_byte_at(addr);
            gr_write_byte_at(addr, (unsigned char)(~val));
        }
    }
}

/* ═══════════════════════════════════════════════════════════════
 * gfx_bitmap
 *
 * Blits a packed 1bpp bitmap (MSB = leftmost pixel, rows
 * stored contiguously, ceil(w/8) bytes per row).
 *
 * For simplicity this uses gfx_pixel per bit — adequate for
 * small sprites.  For large bitmaps consider byte-aligned blitting.
 * ═══════════════════════════════════════════════════════════════ */
void gfx_bitmap(unsigned char x,    unsigned char y,
                unsigned char w,    unsigned char h,
                const unsigned char *data)
{
    unsigned char row, col;
    unsigned char bytes_per_row;
    unsigned char byte_val, bit_mask;
    unsigned int  src_offset;

    if (!data || w == 0 || h == 0) return;

    bytes_per_row = (unsigned char)((w + 7u) / 8u);

    for (row = 0; row < h; row++)
    {
        if ((unsigned int)(y + row) >= LCD_PIXEL_H) break;

        src_offset = (unsigned int)row * (unsigned int)bytes_per_row;

        for (col = 0; col < w; col++)
        {
            if ((unsigned int)(x + col) >= LCD_PIXEL_W) break;

            byte_val = data[src_offset + col / 8u];
            bit_mask = (unsigned char)(0x80u >> (col % 8u));

            gfx_pixel(
                (unsigned char)(x + col),
                (unsigned char)(y + row),
                (byte_val & bit_mask) ? PIXEL_SET : PIXEL_CLEAR
            );
        }
    }
}