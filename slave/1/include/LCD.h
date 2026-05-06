#ifndef LCD_H
#define LCD_H

#include "LCD_CONFIG.h"

/* ─── Bus idle state macro ────────────────────────────────────── *
 * Deasserts all control lines and floats the data bus.
 * Call this after every read/write to leave the bus clean.
 */
#define LCD_BUS_IDLE()  do {        \
    LCD_CE  = 1;                    \
    LCD_RD  = 1;                    \
    LCD_WR  = 1;                    \
    LCD_CD  = 1;                    \
    LCD_DATA = 0xFF;  /* float */   \
} while(0)


/* ─── Assert / deassert helpers ───────────────────────────────── *
 * These make the intent explicit at the call site.
 * "ASSERT" means drive the active level (LOW for these signals).
 */
#define LCD_CE_ASSERT()    (LCD_CE  = 0)
#define LCD_CE_DEASSERT()  (LCD_CE  = 1)

#define LCD_RD_ASSERT()    (LCD_RD  = 0)
#define LCD_RD_DEASSERT()  (LCD_RD  = 1)

#define LCD_WR_ASSERT()    (LCD_WR  = 0)
#define LCD_WR_DEASSERT()  (LCD_WR  = 1)

#define LCD_SEL_COMMAND()  (LCD_CD  = 1)   /* C/D HIGH = command or status */
#define LCD_SEL_DATA()     (LCD_CD  = 0)   /* C/D LOW  = data              */


/* ─── Status bit masks ────────────────────────────────────────── */

#define STA_CMD_RDY       0x01   /* STA0: ready to accept command        */
#define STA_DATA_RDY      0x02   /* STA1: ready for data read/write      */
#define STA_AUTO_READ_RDY 0x04   /* STA2: ready for auto-read byte       */
#define STA_AUTO_WRITE_RDY 0x08  /* STA3: ready for auto-write byte      */
#define STA_CTRL_RDY      0x20   /* STA5: controller operation ready     */
#define STA_ERR           0x40   /* STA6: screen peek/copy error         */
#define STA_BLINK         0x80   /* STA7: blink state                    */



/* Before every normal command or data byte check STA0 AND STA1.  *
 * Before every auto-write byte check STA3.                        *
 * Before every auto-read  byte check STA2.                        */
#define STA_NORMAL_MASK   (STA_CMD_RDY | STA_DATA_RDY)   /* 0x03 */
#define STA_AUTO_W_MASK   (STA_AUTO_WRITE_RDY)            /* 0x08 */
#define STA_AUTO_R_MASK   (STA_AUTO_READ_RDY)             /* 0x04 */


/* ─── Status read macro ───────────────────────────────────────── *
 * Reads one status byte into variable `s` without changing CE/RD *
 * state — the caller controls the bus window.                     *
 *                                                                  *
 * Usage (internal, inside a bus window already open for status):  *
 *   unsigned char s;                                              *
 *   LCD_READ_STATUS(s);                                           *
 */
#define LCD_READ_STATUS(s)  do {    \
    LCD_DATA = 0xFF;                \
    (s) = LCD_DATA;                 \
} while(0)



/* ─── Status wait functions (inline for speed) ────────────────── */

/*
 * LCD_WAIT_READY — spin until STA0=1 AND STA1=1.
 * Call before every normal data write, data read, or command write.
 *
 * The bus window for status read:
 *   C/D HIGH, CE LOW, RD LOW → read bus → CE HIGH, RD HIGH
 */
static void LCD_WAIT_READY(void)
{
    unsigned char s;

    LCD_SEL_COMMAND();   /* C/D = HIGH (status read) */

    do {
        LCD_CE_ASSERT();
        LCD_RD_ASSERT();
        LCD_DATA = 0xFF;           /* float bus before reading */
        s = LCD_DATA;
        LCD_RD_DEASSERT();
        LCD_CE_DEASSERT();
    } while ((s & STA_NORMAL_MASK) != STA_NORMAL_MASK);
}


/*
 * LCD_WAIT_AUTO_WRITE — spin until STA3=1.
 * Call before sending each byte in auto-write mode.
 */
static void LCD_WAIT_AUTO_WRITE(void)
{
    unsigned char s;

    LCD_SEL_COMMAND();

    do {
        LCD_CE_ASSERT();
        LCD_RD_ASSERT();
        LCD_DATA = 0xFF;
        s = LCD_DATA;
        LCD_RD_DEASSERT();
        LCD_CE_DEASSERT();
    } while ((s & STA_AUTO_W_MASK) != STA_AUTO_W_MASK);
}


/*
 * LCD_WAIT_AUTO_READ — spin until STA2=1.
 * Call before reading each byte in auto-read mode.
 */
static void LCD_WAIT_AUTO_READ(void)
{
    unsigned char s;

    LCD_SEL_COMMAND();

    do {
        LCD_CE_ASSERT();
        LCD_RD_ASSERT();
        LCD_DATA = 0xFF;
        s = LCD_DATA;
        LCD_RD_DEASSERT();
        LCD_CE_DEASSERT();
    } while ((s & STA_AUTO_R_MASK) != STA_AUTO_R_MASK);
}


/* ─── Core write macros ───────────────────────────────────────── *
 *                                                                  *
 * These are macros (not functions) so the compiler can inline      *
 * them cleanly — important for timing-sensitive bus cycles.        *
 *                                                                  *
 * IMPORTANT: always call LCD_WAIT_READY() before these.           *
 * The macros do NOT check status themselves — that is deliberate   *
 * so you can batch a 2-byte operand + command without redundant    *
 * status reads between operand bytes.                              *
 */

/*
 * LCD_WRITE_DATA(byte) — write one data byte (C/D LOW).
 *
 * Timing: CE,WR pulse ≥ 80 ns, data setup ≥ 80 ns.
 * On a 12 MHz 8051 one machine cycle ≈ 83 ns — the
 * compiler-generated code naturally meets timing.
 */
#define LCD_WRITE_DATA(byte)  do {  \
    LCD_SEL_DATA();                 \
    LCD_CE_ASSERT();                \
    LCD_WR_ASSERT();                \
    LCD_DATA = (byte);              \
    LCD_WR_DEASSERT();              \
    LCD_CE_DEASSERT();              \
} while(0)

/*
 * LCD_WRITE_CMD(cmd) — write one command byte (C/D HIGH).
 */
#define LCD_WRITE_CMD(cmd)  do {    \
    LCD_SEL_COMMAND();              \
    LCD_CE_ASSERT();                \
    LCD_WR_ASSERT();                \
    LCD_DATA = (cmd);               \
    LCD_WR_DEASSERT();              \
    LCD_CE_DEASSERT();              \
} while(0)


/* ─── Core read macro ─────────────────────────────────────────── */

/*
 * LCD_READ_DATA(var) — read one data byte from display RAM.
 * Access time ≤ 150 ns, output hold ≥ 10 ns.
 * Always call LCD_WAIT_READY() first.
 */
#define LCD_READ_DATA(var)  do {    \
    LCD_SEL_DATA();                 \
    LCD_CE_ASSERT();                \
    LCD_RD_ASSERT();                \
    LCD_DATA = 0xFF;                \
    (var) = LCD_DATA;               \
    LCD_RD_DEASSERT();              \
    LCD_CE_DEASSERT();              \
} while(0)




/* ─── Convenience functions ───────────────────────────────────── */

/*
 * lcd_write_data(byte)
 * Safe wrapper: waits for ready, then writes data.
 * Use this for one-off data bytes.
 */
static void lcd_write_data(unsigned char byte)
{
    LCD_WAIT_READY();
    LCD_WRITE_DATA(byte);
}

/*
 * lcd_write_cmd(cmd)
 * Safe wrapper: waits for ready, then writes command.
 */
static void lcd_write_cmd(unsigned char cmd)
{
    LCD_WAIT_READY();
    LCD_WRITE_CMD(cmd);
}


/*
 * lcd_read_data()
 * Safe wrapper: waits for ready, reads and returns one data byte.
 */
static unsigned char lcd_read_data(void)
{
    unsigned char val;
    LCD_WAIT_READY();
    LCD_READ_DATA(val);
    return val;
}


/*
 * lcd_write_cmd2(lo, hi, cmd)
 * Sends a 2-byte operand followed by a command.
 * T6963C always expects: D1 (low byte) → D2 (high byte) → command.
 *
 * Example:
 *   lcd_write_cmd2(0x00, 0x00, 0x40);  // Set Text Home Address = 0x0000
 *   lcd_write_cmd2(0x1E, 0x00, 0x41);  // Set Text Area = 30 cols
 */
static void lcd_write_cmd2(unsigned char lo,
                            unsigned char hi,
                            unsigned char cmd)
{
    LCD_WAIT_READY();
    LCD_WRITE_DATA(lo);

    LCD_WAIT_READY();
    LCD_WRITE_DATA(hi);

    LCD_WAIT_READY();
    LCD_WRITE_CMD(cmd);
}



/*
 * lcd_write_cmd2w(word, cmd)
 * Same as lcd_write_cmd2 but takes a 16-bit address directly.
 * Splits into low/high bytes automatically.
 *
 * Example:
 *   lcd_write_cmd2w(0x0200, CMD_GR_HOME);  // graphic home at 0x0200
 */
static void lcd_write_cmd2w(unsigned int word, unsigned char cmd)
{
    lcd_write_cmd2((unsigned char)(word & 0xFF),
                   (unsigned char)(word >> 8),
                   cmd);
}


/* ─── Auto-mode write ─────────────────────────────────────────── *
 * Auto mode is fastest for bulk fills (clear screen, bitmaps).    *
 * Sequence:                                                        *
 *   1. lcd_write_cmd2w(start_addr, 0x24)  set address pointer     *
 *   2. lcd_write_cmd(0xB0)                enter auto-write mode   *
 *   3. for each byte: lcd_auto_write(byte)                        *
 *   4. lcd_auto_reset()                   exit auto-write mode    *
 */

/*
 * lcd_auto_write(byte) — send one byte in auto-write mode.
 * Checks STA3, not STA0/STA1.
 */
static void lcd_auto_write(unsigned char byte)
{
    LCD_WAIT_AUTO_WRITE();
    LCD_WRITE_DATA(byte);
}


/*
 * lcd_auto_read() — read one byte in auto-read mode.
 * Checks STA2.
 */
static unsigned char lcd_auto_read(void)
{
    unsigned char val;
    LCD_WAIT_AUTO_READ();
    LCD_READ_DATA(val);
    return val;
}

/*
 * lcd_auto_reset() — exit auto read/write mode.
 * Must be called after every auto read/write block.
 */
static void lcd_auto_reset(void)
{
    LCD_WAIT_READY();
    LCD_WRITE_CMD(0xB2);
}


/* ─── Hardware reset ─── */

/*
 * lcd_hw_reset() — pulse RESET low for ~10 ms then release.
 * Clears column/line counter and display register.
 * Text/graphic home addresses and area settings are retained.
 */
static void lcd_hw_reset(void)
{
    unsigned int i;

    LCD_BUS_IDLE();

    LCD_RESET = 0;
    for (i = 0; i < 30000; i++);   /* ~10 ms at 12 MHz */
    LCD_RESET = 1;
    for (i = 0; i < 30000; i++);   /* settle time       */
}




#endif // LCD_H