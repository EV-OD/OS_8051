#ifndef LCD_COMMAND_H
#define LCD_COMMAND_H

/*
 * LCD controller command definitions for the slave firmware.
 * The X bits in the datasheet table are represented by the
 * canonical command values commonly used with this controller.
 */

/* Register setting */
#define LCD_CMD_SET_CURSOR_POINTER         0x21
#define LCD_CMD_SET_OFFSET_REGISTER        0x22
#define LCD_CMD_SET_ADDRESS_POINTER       0x24

/* Set control word */
#define LCD_CMD_SET_TEXT_HOME_ADDRESS      0x40
#define LCD_CMD_SET_TEXT_AREA              0x41
#define LCD_CMD_SET_GRAPHIC_HOME_ADDRESS   0x42
#define LCD_CMD_SET_GRAPHIC_AREA           0x43

/* Mode set */
#define LCD_CMD_MODE_OR                    0x80
#define LCD_CMD_MODE_EXOR                  0x81
#define LCD_CMD_MODE_AND                   0x83
#define LCD_CMD_MODE_TEXT_ATTRIBUTE        0x84
#define LCD_CMD_MODE_INTERNAL_CG_ROM       0x88
#define LCD_CMD_MODE_EXTERNAL_CG_RAM       0x88

/* Display mode */
#define LCD_CMD_DISPLAY_OFF                0x90
#define LCD_CMD_CURSOR_ON_BLINK_OFF        0x92
#define LCD_CMD_CURSOR_ON_BLINK_ON         0x93
#define LCD_CMD_TEXT_ON_GRAPHIC_OFF        0x94
#define LCD_CMD_TEXT_OFF_GRAPHIC_ON        0x98
#define LCD_CMD_TEXT_ON_GRAPHIC_ON         0x9C

/* Cursor pattern select */
#define LCD_CMD_CURSOR_PATTERN_1_LINE      0xA0
#define LCD_CMD_CURSOR_PATTERN_2_LINE      0xA1
#define LCD_CMD_CURSOR_PATTERN_3_LINE      0xA2
#define LCD_CMD_CURSOR_PATTERN_4_LINE      0xA3
#define LCD_CMD_CURSOR_PATTERN_5_LINE      0xA4
#define LCD_CMD_CURSOR_PATTERN_6_LINE      0xA5
#define LCD_CMD_CURSOR_PATTERN_7_LINE      0xA6
#define LCD_CMD_CURSOR_PATTERN_8_LINE      0xA7

/* Data auto read/write */
#define LCD_CMD_SET_DATA_AUTO_WRITE        0xB0
#define LCD_CMD_SET_DATA_AUTO_READ         0xB1
#define LCD_CMD_AUTO_RESET                 0xB2

/* Data read/write */
#define LCD_CMD_DATA_WRITE_INCREMENT_ADP   0xC0
#define LCD_CMD_DATA_READ_INCREMENT_ADP    0xC1
#define LCD_CMD_DATA_WRITE_DECREMENT_ADP   0xC2
#define LCD_CMD_DATA_READ_DECREMENT_ADP    0xC3
#define LCD_CMD_DATA_WRITE_NONVARIABLE_ADP 0xC4
#define LCD_CMD_DATA_READ_NONVARIABLE_ADP  0xC5

/* Screen operations */
#define LCD_CMD_SCREEN_PEEK                0xE0
#define LCD_CMD_SCREEN_COPY                0xE8

#endif
