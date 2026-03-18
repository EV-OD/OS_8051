#ifndef LCD_H
#define LCD_H

#include "LCD_CONFIG.h"
#include "LCD_COMMAND.h"


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