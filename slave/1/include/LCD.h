#ifndef LCD_H
#define LCD_H
#include <reg51.h>
#define _nop_() __asm__("nop")
// p0.0 - p0.7: LCD data bus
// p2.0 - C/D'
/*
WR=“L”...C/D=“H” : Command write C/D=“L”: Data write
RD=“L”...C/D=“H” : Status read C/D=“L”: Data read
*/

// p2.1 - RD active low
// p2.2 - WR active low
// p2.3 - CE: Chip enable Active Low
// p2.3 - RST: Reset Active Low

/*
18 MD2 I Mode Selection (see below)
19 FS1 I Terminals for selection of font size
*/

// p2.4 - MD2: Mode selection
// p2.5 - FS1: Font size selection

// make a table of font selection based on MD2 and FS1 with proper bar and style
/*

---------------------
| MD2 |  FS1 | Font |
| L   |  L   | 6x8  |
| H   |  L   | 8x8  |
| L   |  H   | 6x8  |
| H   |  H   | 6x8  |
---------------------
*/


/* 
STA7 (MSB)
D7
STA6
D6
STA5
D5
STA4
D4
STA3
D3
STA2
D2
STA1
D1
STA0 (LSB)
D0 


STA0 Check command execution capability 0: Disable
1: Enable
STA1 Check data read / write capability 0: Disable
1: Enable
STA2 Check Auto mode data read capability 0: Disable
1: Enable
STA3 Check Auto mode data write capability 0: Disable
1: Enable
STA4 Not used
STA5 Check controller operation capability 0: Disable
1: Enable
STA6 Error flag. Used for Screen Peek and Screen copy commands. 0: No error
1: Error
STA7 Check the blink condition 0: Display off
1: Normal display 

Note 1: It is necessary to check STA0 and STA1 at the same time.
There is a possibility of erroneous operation due to a hardware interrupt.
Note 2: For most modes STA0 / STA1 are used as a status check.
Note 3: STA2 and STA3 are valid in Auto mode; STA0 and STA1 are invalid. 
*/


// now make a definition for above all pins
#define LCD_DATA P0
#define LCD_CD P2_0
#define LCD_RD P2_1
#define LCD_WR P2_2
#define LCD_CE P2_3
#define LCD_RST P2_4
#define LCD_MD2 P2_5
#define LCD_FS1 P2_6


#define LCD_6x8_FONT 0
#define LCD_8x8_FONT 1


void lcd_init(void);
void lcd_clear(void);
void lcd_write_command(unsigned char cmd);
void lcd_write_data(unsigned char data);
unsigned char lcd_status_read(void);
unsigned int lcd_data_read(void);
void enable_lcd(void);
void disable_lcd(void);
void set_mode_command_or_data(unsigned char is_command);

void wait_ready(void);

void setup_write(void);
void setup_read(void);

void setup_command_write(void);
void setup_data_write(void);
void setup_command_read(void);
void setup_data_read(void);

void font_selection(unsigned char font_type);


// status
unsigned char status_check(void);
unsigned char auto_mode_status_check(void);
unsigned int is_lcd_ready_for_command(void);
unsigned int is_lcd_ready_for_data_or_read(void);

void display_text_only(void);
void display_graphic_only(void);
void display_both(void);



void set_address(unsigned char lowAddr, unsigned char highAddr);




void display_char_text_mode(unsigned char c);

void lcd_clear_ram(void);

void delay_ms(unsigned int ms);
#endif