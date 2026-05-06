/*
 * setup.c — LM3229 / T6963C initialisation and text helpers
 */

#include "setup.h"

/* ─────────────────────────────────────────────────────────────
 * Internal helper: send a 16-bit word (lo first) then a command.
 * Exported via setup.h as lcd_set_addr; reused by graphics.c too.
 * ───────────────────────────────────────────────────────────── */
static void send_word_cmd(unsigned int word, unsigned char cmd)
{
    lcd_write_data((unsigned char)(word & 0x00FFu));
    lcd_write_data((unsigned char)(word >> 8));
    lcd_write_cmd(cmd);
}

/* ─────────────────────────────────────────────────────────────
 * lcd_set_addr
 * ───────────────────────────────────────────────────────────── */
void lcd_set_addr(unsigned int addr)
{
    send_word_cmd(addr, CMD_SET_ADDR_PTR);
}

/* ─────────────────────────────────────────────────────────────
 * lcd_init
 *
 * Full init sequence (T6963C datasheet §Functional Definition):
 *   1. Hardware reset
 *   2. Set text home address + area
 *   3. Set graphic home address + area
 *   4. Mode set (OR mode, internal CG)
 *   5. Clear both RAM areas
 *   6. Enable display (both text + graphic on)
 * ───────────────────────────────────────────────────────────── */
void lcd_init(void)
{
    /* 1. Hardware reset */
    lcd_hw_reset();

    /* 2. Text area */
    send_word_cmd(TXT_HOME, CMD_TXT_HOME_ADDR);
    send_word_cmd((unsigned int)COLS, CMD_TXT_AREA);

    /* 3. Graphic area */
    send_word_cmd(GR_HOME, CMD_GR_HOME_ADDR);
    send_word_cmd((unsigned int)COLS, CMD_GR_AREA);

    /* 4. OR mode + internal CG ROM */
    lcd_write_cmd(CMD_MODE_OR | CMD_MODE_INT_CG);

    /* 5. Blank both RAM planes */
    lcd_clear_all();

    /* 6. Turn on text + graphic display, cursor off */
    lcd_write_cmd(CMD_DISP_BOTH_ON);
}

/* ─────────────────────────────────────────────────────────────
 * lcd_clear_text
 * ───────────────────────────────────────────────────────────── */
void lcd_clear_text(void)
{
    unsigned int i;

    lcd_set_addr(TXT_HOME);
    lcd_write_cmd(CMD_AUTO_WRITE);

    for (i = 0; i < TXT_SIZE; i++)
        lcd_auto_write(0x00u);

    lcd_auto_reset();
}

/* ─────────────────────────────────────────────────────────────
 * lcd_clear_graphic
 * ───────────────────────────────────────────────────────────── */
void lcd_clear_graphic(void)
{
    unsigned int i;

    lcd_set_addr(GR_HOME);
    lcd_write_cmd(CMD_AUTO_WRITE);

    for (i = 0; i < GR_SIZE; i++)
        lcd_auto_write(0x00u);

    lcd_auto_reset();
}

/* ─────────────────────────────────────────────────────────────
 * lcd_clear_all
 * ───────────────────────────────────────────────────────────── */
void lcd_clear_all(void)
{
    lcd_clear_text();
    lcd_clear_graphic();
}

/* ─────────────────────────────────────────────────────────────
 * lcd_set_display_mode
 * ───────────────────────────────────────────────────────────── */
void lcd_set_display_mode(unsigned char mode)
{
    lcd_write_cmd(mode);
}

/* ─────────────────────────────────────────────────────────────
 * lcd_putchar
 * ───────────────────────────────────────────────────────────── */
void lcd_putchar(unsigned char col, unsigned char row, unsigned char ch)
{
    unsigned int addr;

    if (col >= COLS || row >= ROWS) return;   /* bounds check */

    addr = TXT_HOME
         + (unsigned int)row * (unsigned int)COLS
         + (unsigned int)col;

    lcd_set_addr(addr);
    lcd_write_data(ch);
    lcd_write_cmd(CMD_DATA_WRITE_INC);
}

/* ─────────────────────────────────────────────────────────────
 * lcd_puts
 * ───────────────────────────────────────────────────────────── */
void lcd_puts(unsigned char col, unsigned char row,
              const unsigned char *str)
{
    unsigned char c = col;
    unsigned char r = row;
    unsigned int  addr;

    if (!str) return;

    addr = TXT_HOME
         + (unsigned int)r * (unsigned int)COLS
         + (unsigned int)c;

    lcd_set_addr(addr);
    lcd_write_cmd(CMD_AUTO_WRITE);

    while (*str)
    {
        lcd_auto_write(*str++);
        c++;
        if (c >= COLS)          /* soft wrap */
        {
            c = 0;
            r++;
            if (r >= ROWS) break;

            /* Restart auto-write at new row start */
            lcd_auto_reset();
            addr = TXT_HOME + (unsigned int)r * (unsigned int)COLS;
            lcd_set_addr(addr);
            lcd_write_cmd(CMD_AUTO_WRITE);
        }
    }

    lcd_auto_reset();
}

/* ─────────────────────────────────────────────────────────────
 * lcd_puts_pgm  (code-memory string)
 * ───────────────────────────────────────────────────────────── */
void lcd_puts_pgm(unsigned char col, unsigned char row,
                  const char *str)
{
    /* On 8051 with small/large model, const char* already lives in
     * code space if declared with 'code' keyword.  Cast is safe. */
    lcd_puts(col, row, (const unsigned char *)str);
}