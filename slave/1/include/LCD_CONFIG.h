#ifndef LCD_CONFIG_H
#define LCD_CONFIG_H

#include <reg51.h>

#define LCD_DATA P0

#define LCD_CD   P2_0
#define LCD_RD   P2_1
#define LCD_WR   P2_2
#define LCD_CE   P2_3
#define LCD_RST  P2_4
#define LCD_MD2  P2_5
#define LCD_FS1  P2_6

#define LCD_6x8_FONT 0u
#define LCD_8x8_FONT 1u

#define LCD_TEXT_HOME_LOW        0x00u
#define LCD_TEXT_HOME_HIGH       0x00u
#define LCD_TEXT_AREA_COLUMNS    40u

#define LCD_GRAPHIC_HOME_LOW     0x00u
#define LCD_GRAPHIC_HOME_HIGH    0x10u
#define LCD_GRAPHIC_AREA_COLUMNS 40u

#endif
