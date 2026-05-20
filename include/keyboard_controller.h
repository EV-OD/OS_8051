#ifndef KEYBOARD_CONTROLLER_H
#define KEYBOARD_CONTROLLER_H

#include <reg51.h>
#define KEYSET P2
#define KEY_START P2_0
#define KEY_NEXT P2_1

#define LE P3_0

#define INTERRUPT_ENABLE P3_2


unsigned char saved_key = 0x00;
static volatile __idata unsigned char start_key = 0x00;
static volatile __idata unsigned char next_key = 0x00;

typedef enum  {
    KEY_NONE = 0x00,
    START = 0x01,
    NEXT = 0x02
} KEY_STATE;

void keyboard_init(void){
    KEYSET = 0xFF; // Set all pins as input with pull-ups
    LE = 1;  // Latch Enable high to read key states

    IT0 = 0; // Configure INT0 for level-triggered mode
    EX0 = 1;
    EA  = 1;
}

void process_key(){
    start_key = KEY_START;
    next_key = KEY_NEXT; 
}


void external_button_isr(void) __interrupt (0) {
    // 1. INSTANTLY drop LE to lock the hardware outputs!
    // Even if the user releases the button right now, the data is frozen inside the 74HC373.
    LE = 0; 
    
    // 2. Read the frozen button data safely from the bus
    saved_key = KEYSET;
    process_key();

    // 4. Clear the interrupt condition
    // Wait until the user actually removes their finger from the physical button 
    // to prevent continuous re-triggering.
    LE = 1; // Open latch back up to check live inputs
    while(KEYSET != 0x00); // Wait for release
}

KEY_STATE isKeyPressed(){
    if (saved_key == 0x00) {    
        return KEY_NONE;
    } else if (saved_key == 0x01) {
        saved_key = 0x00; // Clear after reading
        return START;
    } else if (saved_key == 0x02) {
        saved_key = 0x00; // Clear after reading
        return NEXT;
    }
    return KEY_NONE;
}


#endif /* KEYBOARD_CONTROLLER_H */