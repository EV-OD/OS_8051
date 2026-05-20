/*
 * graphics.h — LM3229 pixel-level drawing API
 *
 * All coordinates are zero-based:
 *   x: 0 … (LCD_PIXEL_W - 1)   i.e. 0–239
 *   y: 0 … (LCD_PIXEL_H - 1)   i.e. 0–127
 *
 * All functions clip silently — pixels outside the screen boundary
 * are discarded without error.
 *
 * Dependencies:
 *   setup.h  → t6963c.h
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "setup.h"

/* ═══════════════════════════════════════════════════════════════
 * PIXEL ADDRESS MATH
 *
 * The graphic RAM is a flat byte array, one bit per pixel.
 * MSB of each byte is the leftmost pixel in that 8-pixel group.
 *
 *   byte address = GR_HOME  +  y * COLS  +  (x / 8)
 *   bit position = 7 - (x % 8)          ← MSB = left
 *
 * The BIT SET / BIT RESET commands (0xF8|bit and 0xF0|bit)
 * work on the byte at the current ADP — the fastest way to
 * toggle a single pixel without read-modify-write in the CPU.
 * ═══════════════════════════════════════════════════════════════ */

/* Precomputed address of the byte containing pixel (x,y) */
#define GR_BYTE_ADDR(x, y) \
    (gfx_gr_home + (unsigned int)(y) * (unsigned int)(COLS) + (unsigned int)(x) / 8u)

/* Bit position within that byte (0 = LSB, 7 = MSB / leftmost) */
#define GR_BIT_POS(x)   (7u - ((unsigned int)(x) % 8u))

/* Current graphic draw base (home address). Used by GR_BYTE_ADDR(). */
extern __idata unsigned int gfx_gr_home;

/* Select which graphic page drawing functions write into (0 or 1). */
void gfx_set_draw_page(unsigned char page);

/* ═══════════════════════════════════════════════════════════════
 * PRIMITIVE DRAWING FUNCTIONS
 * ═══════════════════════════════════════════════════════════════ */

/*
 * gfx_pixel(x, y, state)
 * Set (state=PIXEL_SET) or clear (state=PIXEL_CLEAR) a single pixel.
 * Uses BIT SET/RESET command — no RAM read needed.
 */
void gfx_pixel(unsigned char x, unsigned char y, unsigned char state);

/*
 * gfx_hline(x1, x2, y)
 * Fast horizontal line.  Paints full bytes in the middle,
 * masked bytes at the ends — much faster than gfx_pixel loop.
 */
void gfx_hline(unsigned char x1, unsigned char x2, unsigned char y);

/*
 * gfx_vline(x, y1, y2)
 * Vertical line.  One BIT SET per row — column stride = COLS bytes.
 */
void gfx_vline(unsigned char x, unsigned char y1, unsigned char y2);

/*
 * gfx_line(x1, y1, x2, y2)
 * Arbitrary line using Bresenham's algorithm.
 */
void gfx_line(unsigned char x1, unsigned char y1,
              unsigned char x2, unsigned char y2);

/*
 * gfx_rect(x1, y1, x2, y2)
 * Hollow rectangle (four lines).
 */
void gfx_rect(unsigned char x1, unsigned char y1,
              unsigned char x2, unsigned char y2);

/*
 * gfx_fill_rect(x1, y1, x2, y2)
 * Solid filled rectangle — uses auto-write for speed.
 */
void gfx_fill_rect(unsigned char x1, unsigned char y1,
                   unsigned char x2, unsigned char y2);

/*
 * gfx_circle(cx, cy, r)
 * Hollow circle using Bresenham midpoint algorithm.
 * Draws all 8 octants symmetrically.
 */
void gfx_circle(unsigned char cx, unsigned char cy, unsigned char r);

/*
 * gfx_fill_circle(cx, cy, r)
 * Solid filled circle — fills each pair of symmetric rows
 * with gfx_hline.
 */
void gfx_fill_circle(unsigned char cx, unsigned char cy, unsigned char r);

/*
 * gfx_clear_rect(x1, y1, x2, y2)
 * Fill a rectangular region with 0 (pixels off).
 * Same as gfx_fill_rect but clears instead of sets.
 */
void gfx_clear_rect(unsigned char x1, unsigned char y1,
                    unsigned char x2, unsigned char y2);

/*
 * gfx_invert_rect(x1, y1, x2, y2)
 * XOR every pixel in a rectangle — toggles set↔clear.
 * Useful for highlighting / rubber-band selection.
 */
void gfx_invert_rect(unsigned char x1, unsigned char y1,
                     unsigned char x2, unsigned char y2);

#endif /* GRAPHICS_H */