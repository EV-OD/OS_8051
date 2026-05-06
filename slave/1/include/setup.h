/*
 * setup.h — LM3229 / T6963C display configuration and initialisation
 *
 * Defines the memory layout, font geometry, and all high-level
 * setup operations.  Include this (not t6963c.h directly) in your
 * application files.
 *
 * Dependencies:
 *   t6963c.h  — bus-level macros and pin definitions
 */

#ifndef SETUP_H
#define SETUP_H

#include "LCD.h"

#define LCD_PIXEL_W     240     /* physical pixels wide          */
#define LCD_PIXEL_H     128     /* physical pixels tall          */

/*
 * Font size — choose ONE by uncommenting.
 * Affects FONT_W, COLS, and GR_HOME calculations below.
 *
 * 6×8: MD2=L, FS1=H on hardware → 40 text columns
 * 8×8: MD2=H, FS1=L on hardware → 30 text columns
 */
/* #define FONT_6x8 */
#define FONT_8x8

#ifdef FONT_6x8
  #define FONT_W   6
  #define FONT_H   8
#else
  #define FONT_W   8
  #define FONT_H   8
#endif

#define COLS    (LCD_PIXEL_W / FONT_W)   /* text columns: 30 or 40 */
#define ROWS    (LCD_PIXEL_H / FONT_H)   /* text rows:    always 16 */

/* ═══════════════════════════════════════════════════════════════
 * 2. RAM LAYOUT
 *
 *   0x0000 ── TEXT AREA  ── (COLS × ROWS) bytes
 *   0x0200 ── GRAPHIC AREA ─ (COLS × LCD_PIXEL_H) bytes
 *
 * With 8×8, COLS=30:
 *   Text    = 30 × 16 = 480 bytes  → 0x0000–0x01EF
 *   Graphic = 30 × 128 = 3840 bytes → 0x0200–0x10FF
 *
 * With 6×8, COLS=40:
 *   Text    = 40 × 16 = 640 bytes  → 0x0000–0x027F
 *   Graphic = 40 × 128 = 5120 bytes → 0x0300–0x163F
 *   (GR_HOME adjusted automatically below)
 * ═══════════════════════════════════════════════════════════════ */

#define TXT_HOME    0x0000u

/* Graphic starts right after text area, rounded up to next 0x100 */
#define TXT_SIZE    ((unsigned int)(COLS) * (unsigned int)(ROWS))

#ifdef FONT_6x8
  #define GR_HOME   0x0300u    /* 640 bytes text; next clean page */
#else
  #define GR_HOME   0x0200u    /* 480 bytes text; next clean page */
#endif

#define GR_SIZE     ((unsigned int)(COLS) * (unsigned int)(LCD_PIXEL_H))

/* ═══════════════════════════════════════════════════════════════
 * 3. T6963C COMMAND CODES
 * ═══════════════════════════════════════════════════════════════ */

/* Register setting */
#define CMD_SET_CURSOR_PTR    0x21u
#define CMD_SET_OFFSET_REG    0x22u
#define CMD_SET_ADDR_PTR      0x24u

/* Control word */
#define CMD_TXT_HOME_ADDR     0x40u
#define CMD_TXT_AREA          0x41u
#define CMD_GR_HOME_ADDR      0x42u
#define CMD_GR_AREA           0x43u

/* Mode set */
#define CMD_MODE_OR           0x80u   /* OR  text + graphic       */
#define CMD_MODE_EXOR         0x81u   /* XOR text + graphic       */
#define CMD_MODE_AND          0x83u   /* AND text + graphic       */
#define CMD_MODE_ATTR         0x84u   /* text attribute mode      */
#define CMD_MODE_INT_CG       0x80u   /* internal CG ROM (|with mode) */
#define CMD_MODE_EXT_CG       0x88u   /* external CG RAM          */

/* Display mode — build with OR */
#define CMD_DISP_OFF          0x90u
#define CMD_DISP_CURSOR_SOLID 0x92u   /* cursor on, no blink      */
#define CMD_DISP_CURSOR_BLINK 0x93u   /* cursor on + blink        */
#define CMD_DISP_TXT_ON       0x94u   /* text ON,  graphic OFF    */
#define CMD_DISP_GR_ON        0x98u   /* text OFF, graphic ON     */
#define CMD_DISP_BOTH_ON      0x9Cu   /* text ON,  graphic ON     */

/* Cursor pattern */
#define CMD_CURSOR_1LINE      0xA0u
#define CMD_CURSOR_2LINE      0xA1u
#define CMD_CURSOR_8LINE      0xA7u

/* Data auto read/write */
#define CMD_AUTO_WRITE        0xB0u
#define CMD_AUTO_READ         0xB1u
#define CMD_AUTO_RESET        0xB2u

/* Data read/write (single byte) */
#define CMD_DATA_WRITE_INC    0xC0u   /* write + increment ADP    */
#define CMD_DATA_READ_INC     0xC1u   /* read  + increment ADP    */
#define CMD_DATA_WRITE_DEC    0xC2u   /* write + decrement ADP    */
#define CMD_DATA_READ_DEC     0xC3u
#define CMD_DATA_WRITE_NOP    0xC4u   /* write, ADP unchanged     */
#define CMD_DATA_READ_NOP     0xC5u

/* Bit set/reset — OR with bit number 0–7 */
#define CMD_BIT_RESET         0xF0u   /* CMD_BIT_RESET | bit_num  */
#define CMD_BIT_SET           0xF8u   /* CMD_BIT_SET   | bit_num  */

/* ═══════════════════════════════════════════════════════════════
 * 4. COLOUR / PIXEL CONSTANTS
 * ═══════════════════════════════════════════════════════════════ */

#define PIXEL_SET    1
#define PIXEL_CLEAR  0

/* ═══════════════════════════════════════════════════════════════
 * 5. PUBLIC API — setup.c
 * ═══════════════════════════════════════════════════════════════ */

/*
 * lcd_init()
 * Full hardware + software initialisation sequence.
 * Call once at power-on before any other LCD function.
 */
void lcd_init(void);

/*
 * lcd_set_addr(addr)
 * Move the Address Data Pointer to an absolute RAM address.
 * Used internally by graphics and text functions.
 */
void lcd_set_addr(unsigned int addr);

/*
 * lcd_clear_text()
 * Fill the entire text area with 0x00 (space / blank character).
 */
void lcd_clear_text(void);

/*
 * lcd_clear_graphic()
 * Fill the entire graphic area with 0x00 (all pixels off).
 */
void lcd_clear_graphic(void);

/*
 * lcd_clear_all()
 * Clear both text and graphic areas.
 */
void lcd_clear_all(void);

/*
 * lcd_set_display_mode(mode)
 * Switch display between text-only, graphic-only, or combined.
 * Pass one of: CMD_DISP_TXT_ON, CMD_DISP_GR_ON, CMD_DISP_BOTH_ON
 */
void lcd_set_display_mode(unsigned char mode);

/*
 * lcd_putchar(col, row, ch)
 * Write a single character at a text grid position.
 * col: 0 to (COLS-1),  row: 0 to (ROWS-1)
 */
void lcd_putchar(unsigned char col, unsigned char row, unsigned char ch);

/*
 * lcd_puts(col, row, str)
 * Write a null-terminated string starting at (col, row).
 * Wraps to the next row at COLS. Does not scroll.
 */
void lcd_puts(unsigned char col, unsigned char row,
              const unsigned char *str);

/*
 * lcd_puts_pgm(col, row, str)
 * Same as lcd_puts but source string is in code memory (far/const).
 * Handy for literal strings on memory-constrained 8051 variants.
 */
void lcd_puts_pgm(unsigned char col, unsigned char row,
                  const char *str);

#endif /* SETUP_H */