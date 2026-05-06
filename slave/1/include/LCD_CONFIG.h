#ifndef  LCD_CONFIG_H
#define  LCD_CONFIG_H

#include <reg51.h>

#define LCD_CD P2_0
#define LCD_RD     P2_1
#define LCD_WR     P2_2


#define LCD_CE     P2_3
#define LCD_RESET    P2_4

#define LCD_MD2    P2_5
#define LCD_FS1    P2_6

#define LCD_DATA P0


/*
    8x8
    MD2 = 1
    FS1 = 0
*/
#define FONT_SIZE_8x8 10 

#endif // LCD_CONFIG_H