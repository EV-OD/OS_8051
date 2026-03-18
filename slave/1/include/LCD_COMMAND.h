#ifndef LCD_COMMAND_H
#define LCD_COMMAND_H

/* Register setting */
#define LCD_CMD_SET_CURSOR_POINTER         0x21u
#define LCD_CMD_SET_OFFSET_REGISTER        0x22u
#define LCD_CMD_SET_ADDRESS_POINTER        0x24u

/* Set control word */
#define LCD_CMD_SET_TEXT_HOME_ADDRESS      0x40u
#define LCD_CMD_SET_TEXT_AREA              0x41u
#define LCD_CMD_SET_GRAPHIC_HOME_ADDRESS   0x42u
#define LCD_CMD_SET_GRAPHIC_AREA           0x43u

/* Mode set */
#define LCD_CMD_MODE_OR                    0x80u
#define LCD_CMD_MODE_EXOR                  0x81u
#define LCD_CMD_MODE_AND                   0x83u
#define LCD_CMD_MODE_TEXT_ATTRIBUTE        0x84u
#define LCD_CMD_MODE_INTERNAL_CG_ROM       0x88u
#define LCD_CMD_MODE_EXTERNAL_CG_RAM       0x88u

/* Display mode */
#define LCD_CMD_DISPLAY_OFF                0x90u
#define LCD_CMD_CURSOR_ON_BLINK_OFF        0x92u
#define LCD_CMD_CURSOR_ON_BLINK_ON         0x93u
#define LCD_CMD_TEXT_ON_GRAPHIC_OFF        0x94u
#define LCD_CMD_TEXT_OFF_GRAPHIC_ON        0x98u
#define LCD_CMD_TEXT_ON_GRAPHIC_ON         0x9Cu

/* Cursor pattern select */
#define LCD_CMD_CURSOR_PATTERN_1_LINE      0xA0u
#define LCD_CMD_CURSOR_PATTERN_2_LINE      0xA1u
#define LCD_CMD_CURSOR_PATTERN_3_LINE      0xA2u
#define LCD_CMD_CURSOR_PATTERN_4_LINE      0xA3u
#define LCD_CMD_CURSOR_PATTERN_5_LINE      0xA4u
#define LCD_CMD_CURSOR_PATTERN_6_LINE      0xA5u
#define LCD_CMD_CURSOR_PATTERN_7_LINE      0xA6u
#define LCD_CMD_CURSOR_PATTERN_8_LINE      0xA7u

/* Data auto read/write */
#define LCD_CMD_SET_DATA_AUTO_WRITE        0xB0u
#define LCD_CMD_SET_DATA_AUTO_READ         0xB1u
#define LCD_CMD_AUTO_RESET                 0xB2u

/* Data read/write */
#define LCD_CMD_DATA_WRITE_INCREMENT_ADP   0xC0u
#define LCD_CMD_DATA_READ_INCREMENT_ADP    0xC1u
#define LCD_CMD_DATA_WRITE_DECREMENT_ADP   0xC2u
#define LCD_CMD_DATA_READ_DECREMENT_ADP    0xC3u
#define LCD_CMD_DATA_WRITE_NONVARIABLE_ADP 0xC4u
#define LCD_CMD_DATA_READ_NONVARIABLE_ADP  0xC5u

/* Screen operations */
#define LCD_CMD_SCREEN_PEEK                0xE0u
#define LCD_CMD_SCREEN_COPY                0xE8u

/* Status masks */
#define LCD_STATUS_COMMAND_READY           0x03u
#define LCD_STATUS_DATA_READY              0x03u
#define LCD_STATUS_AUTO_MODE_READY         0x0Cu
#define LCD_STATUS_AUTO_WRITE_READY        0x08u

#endif
