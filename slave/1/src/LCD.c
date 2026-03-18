#include "LCD.h"

#include <mcs51/compiler.h>

static void lcd_write_word(unsigned char low_byte, unsigned char high_byte);
static unsigned char lcd_read_status_mask(unsigned char mask);

const LCDInitConfig LCD_INIT_DEFAULT = {
    LCD_6x8_FONT,
    LCD_TEXT_HOME_LOW,
    LCD_TEXT_HOME_HIGH,
    (unsigned char)LCD_TEXT_AREA_COLUMNS,
    0x00u,
    LCD_GRAPHIC_HOME_LOW,
    LCD_GRAPHIC_HOME_HIGH,
    (unsigned char)LCD_GRAPHIC_AREA_COLUMNS,
    0x00u,
    LCD_CMD_TEXT_ON_GRAPHIC_OFF,
    1u
};

const LCDInitConfig LCD_INIT_TEXT_8X8 = {
    LCD_8x8_FONT,
    LCD_TEXT_HOME_LOW,
    LCD_TEXT_HOME_HIGH,
    (unsigned char)LCD_TEXT_AREA_COLUMNS,
    0x00u,
    LCD_GRAPHIC_HOME_LOW,
    LCD_GRAPHIC_HOME_HIGH,
    (unsigned char)LCD_GRAPHIC_AREA_COLUMNS,
    0x00u,
    LCD_CMD_TEXT_ON_GRAPHIC_OFF,
    1u
};

const LCDInitConfig LCD_INIT_BOTH_6X8 = {
    LCD_6x8_FONT,
    LCD_TEXT_HOME_LOW,
    LCD_TEXT_HOME_HIGH,
    (unsigned char)LCD_TEXT_AREA_COLUMNS,
    0x00u,
    LCD_GRAPHIC_HOME_LOW,
    LCD_GRAPHIC_HOME_HIGH,
    (unsigned char)LCD_GRAPHIC_AREA_COLUMNS,
    0x00u,
    LCD_CMD_TEXT_ON_GRAPHIC_ON,
    1u
};

void lcd_init(const LCDInitConfig *config)
{
    const LCDInitConfig *active_config;

    active_config = (config != 0) ? config : &LCD_INIT_DEFAULT;

    lcd_clear();

    font_selection(active_config->font_type);

    lcd_write_word(active_config->text_home_low, active_config->text_home_high);
    lcd_write_command(LCD_CMD_SET_TEXT_HOME_ADDRESS);

    lcd_write_word(active_config->text_area_low, active_config->text_area_high);
    lcd_write_command(LCD_CMD_SET_TEXT_AREA);

    lcd_write_word(active_config->graphic_home_low, active_config->graphic_home_high);
    lcd_write_command(LCD_CMD_SET_GRAPHIC_HOME_ADDRESS);

    lcd_write_word(active_config->graphic_area_low, active_config->graphic_area_high);
    lcd_write_command(LCD_CMD_SET_GRAPHIC_AREA);

    lcd_write_command(active_config->display_mode);

    if (active_config->clear_ram) {
        lcd_clear_ram();
    }
}

void lcd_clear(void)
{
    LCD_RST = 0;
    NOP();
    LCD_RST = 1;
}

void lcd_write_command(unsigned char cmd)
{
    wait_ready();
    set_mode_command_or_data(1);
    LCD_DATA = cmd;
    setup_write();
    NOP();
    LCD_WR = 1;
    disable_lcd();
}

void lcd_write_data(unsigned char data)
{
    wait_ready();
    set_mode_command_or_data(0);
    LCD_DATA = data;
    setup_write();
    NOP();
    LCD_WR = 1;
    disable_lcd();
}

unsigned char lcd_status_read(void)
{
    unsigned char status;

    LCD_DATA = 0xFFu;
    set_mode_command_or_data(1);
    setup_read();
    status = LCD_DATA;
    LCD_RD = 1;
    disable_lcd();

    return status;
}

void wait_ready(void)
{
    while (!lcd_read_status_mask(LCD_STATUS_COMMAND_READY)) {
    }
}

unsigned int lcd_data_read(void)
{
    unsigned char data;

    LCD_DATA = 0xFFu;
    set_mode_command_or_data(0);
    setup_read();
    data = LCD_DATA;
    LCD_RD = 1;
    disable_lcd();

    return data;
}

void enable_lcd(void)
{
    LCD_CE = 0;
}

void disable_lcd(void)
{
    LCD_CE = 1;
}

void set_mode_command_or_data(unsigned char is_command)
{
    LCD_CD = is_command ? 1 : 0;
}

void setup_write(void)
{
    enable_lcd();
    LCD_WR = 0;
    LCD_RD = 1;
}

void setup_read(void)
{
    enable_lcd();
    LCD_RD = 0;
    LCD_WR = 1;
}

void setup_command_write(void)
{
    set_mode_command_or_data(1);
    setup_write();
}

void setup_data_write(void)
{
    set_mode_command_or_data(0);
    setup_write();
}

void setup_command_read(void)
{
    set_mode_command_or_data(1);
    setup_read();
}

void setup_data_read(void)
{
    set_mode_command_or_data(0);
    setup_read();
}

unsigned char status_check(void)
{
    return (unsigned char)(is_lcd_ready_for_command() && is_lcd_ready_for_data_or_read());
}

unsigned char auto_mode_status_check(void)
{
    return lcd_read_status_mask(LCD_STATUS_AUTO_MODE_READY);
}

unsigned int is_lcd_ready_for_command(void)
{
    return lcd_read_status_mask(LCD_STATUS_COMMAND_READY);
}

unsigned int is_lcd_ready_for_data_or_read(void)
{
    return lcd_read_status_mask(LCD_STATUS_DATA_READY);
}

void display_text_only(void)
{
    lcd_write_command(LCD_CMD_TEXT_ON_GRAPHIC_OFF);
}

void display_graphic_only(void)
{
    lcd_write_command(LCD_CMD_TEXT_OFF_GRAPHIC_ON);
}

void display_both(void)
{
    lcd_write_command(LCD_CMD_TEXT_ON_GRAPHIC_ON);
}

void set_address(unsigned char lowAddr, unsigned char highAddr)
{
    lcd_write_word(lowAddr, highAddr);
    lcd_write_command(LCD_CMD_SET_ADDRESS_POINTER);
}

void display_char_text_mode(unsigned char c)
{
    lcd_write_data(c);
    lcd_write_command(LCD_CMD_DATA_WRITE_INCREMENT_ADP);
}

void font_selection(unsigned char font_type)
{
    switch (font_type) {
    case LCD_6x8_FONT:
        LCD_MD2 = 0;
        LCD_FS1 = 1;
        break;
    case LCD_8x8_FONT:
        LCD_MD2 = 1;
        LCD_FS1 = 0;
        break;
    default:
        break;
    }
}

void lcd_clear_ram(void)
{
    unsigned int i;

    set_address(LCD_TEXT_HOME_LOW, LCD_TEXT_HOME_HIGH);
    lcd_write_command(LCD_CMD_SET_DATA_AUTO_WRITE);

    for (i = 0; i < 8192u; ++i) {
        while ((lcd_status_read() & LCD_STATUS_AUTO_WRITE_READY) == 0u) {
        }
        lcd_write_data(0x00u);
    }

    lcd_write_command(LCD_CMD_AUTO_RESET);
}

void delay_ms(unsigned int ms)
{
    unsigned int i;
    unsigned int j;

    for (i = 0; i < ms; ++i) {
        for (j = 0; j < 120u; ++j) {
        }
    }
}

static void lcd_write_word(unsigned char low_byte, unsigned char high_byte)
{
    lcd_write_data(low_byte);
    lcd_write_data(high_byte);
}

static unsigned char lcd_read_status_mask(unsigned char mask)
{
    return (unsigned char)((lcd_status_read() & mask) == mask);
}