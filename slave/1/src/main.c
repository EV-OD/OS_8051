#include "LCD.h"

void main(void)
{
    P2 = 0x00;
    lcd_init();
    for(;;){
        display_char_text_mode('R');
        display_char_text_mode('A');
        display_char_text_mode('B');
        display_char_text_mode('I');
        display_char_text_mode('N');
    }
}
