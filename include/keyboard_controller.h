#ifndef KEYBOARD_CONTROLLER_H
#define KEYBOARD_CONTROLLER_H

#include <reg51.h>
#define KEYSET P2

// Key mapping (P2)
#define KEY_UP P2_0
#define KEY_LEFT P2_1
#define KEY_RIGHT P2_2
#define KEY_DOWN P2_3

#define KEY_MASK 0x0F

#define LE P3_0

#define INTERRUPT_ENABLE P3_2


unsigned char saved_key = 0x00;

typedef enum  {
    KEY_NONE = 0x00,
    UP = 0x01,
    LEFT = 0x02,
    RIGHT = 0x04,
    DOWN = 0x08,

    // Backward-compatible names (old mapping on P2.0/P2.1)
    START = UP,
    NEXT = LEFT
} KEY_STATE;

void keyboard_init(void){
    KEYSET = 0xFF; // Set all pins as input with pull-ups
    LE = 1;  // Latch Enable high to read key states

    IT0 = 0; // Configure INT0 for level-triggered mode
    EX0 = 1;
    EA  = 1;
}


void external_button_isr(void) __interrupt (0) {
    // 1. INSTANTLY drop LE to lock the hardware outputs!
    // Even if the user releases the button right now, the data is frozen inside the 74HC373.
    LE = 0; 
    
    // 2. Read the frozen button data safely from the bus
    saved_key = (KEYSET & KEY_MASK);

    // 4. Clear the interrupt condition
    // Wait until the user actually removes their finger from the physical button 
    // to prevent continuous re-triggering.
    LE = 1; // Open latch back up to check live inputs
    while((KEYSET & KEY_MASK) != 0x00); // Wait for release
}

KEY_STATE isKeyPressed(){
    if (saved_key == 0x00) {    
        return KEY_NONE;
    } else if (saved_key & 0x01) {
        saved_key = 0x00; // Clear after reading
        return UP;
    } else if (saved_key & 0x02) {
        saved_key = 0x00; // Clear after reading
        return LEFT;
    } else if (saved_key & 0x04) {
        saved_key = 0x00;
        return RIGHT;
    } else if (saved_key & 0x08) {
        saved_key = 0x00;
        return DOWN;
    }
    return KEY_NONE;
}

/* Read and clear the raw latched key bitmask (can include multiple keys). */
#define keyboard_read_mask(dst_mask) do { \
    (dst_mask) = saved_key;             \
    saved_key = 0x00;                  \
} while (0)


#endif /* KEYBOARD_CONTROLLER_H */