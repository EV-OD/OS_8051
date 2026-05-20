#ifndef GPU_SLAVE_H
#define GPU_SLAVE_H

#include <reg51.h>

/* Pin Definitions */
#define GPU_DATA P1
#define STB P3_2
#define BUSY P3_3



#define CMD_CLS      0x01
#define CMD_LINE     0x02
#define CMD_CIRCLE   0x03
#define CMD_RECT     0x04
#define CMD_BITMAP   0x05
#define CMD_PIXEL    0x06
#define CMD_FIXED    0x07

// Extended rectangle commands (x1, y1, x2, y2)
#define CMD_FILL_RECT   0x08
#define CMD_CLEAR_RECT  0x09
#define CMD_INVERT_RECT 0x0A

// Page flipping / double-buffer control
#define CMD_SET_DRAW_PAGE    0x0B
#define CMD_SET_DISPLAY_PAGE 0x0C
#define CMD_SWAP_PAGES       0x0D
#define CMD_CLEAR_DRAW_PAGE  0x0E

/* ═══════════════════════════════════════════════════════════════
 * CORE COMMUNICATION & CONTROL
 * ═══════════════════════════════════════════════════════════════ */

/**
 * Initializes the GPU's state and pins.
 */
void gpu_init(void);

/**
 * The hardware handshake to read a single byte from the bus.
 * Updates the BUSY line to coordinate with Master's STB.
 */
unsigned char gpu_receive_byte(void);

/**
 * The main listener loop that decodes incoming Command IDs.
 */
void gpu_process_commands(void);

#endif /* GPU_SLAVE_H */