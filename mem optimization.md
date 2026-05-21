# Memory Optimization

Simple approaches used to keep RAM usage low on the 8051.

## 1. Put constant data in code memory

Static strings, maps, sprites, and lookup tables are stored with `__code` where possible.

Examples:

- UI strings are passed as `const __code char *`.
- The rogue map is stored in flash as `ROGUE_MAP`.
- Obstacle width/height tables use `__code`.
- The slave-side fixed bitmap uses `__code`.

This keeps internal RAM free for active game state.

## 2. Use `unsigned char` for small values

Most positions, counters, flags, commands, and dimensions fit in 8 bits, so the code uses `unsigned char` instead of `int`.

This matters because 8051 internal RAM is very small, and SDCC may use extra helper code/registers for larger arithmetic.

## 3. Pack related state into one byte

Maze obstacles store multiple fields in one byte:

- bits `0..2` - width index
- bits `3..5` - height index
- bits `6..7` - shape type

So each obstacle only needs:

- `obs_x[i]`
- `obs_y[i]`
- `obs_p[i]`

The actual width/height values are read from flash lookup tables.

## 4. Use bitsets instead of arrays

Conway's Game of Life stores the 8x8 board as 8 bytes:

- one byte per row
- one bit per cell

The next generation also uses 8 bytes. This avoids a 64-byte cell array.

## 5. Stream data instead of buffering

Text and bitmap data are sent from master to slave byte by byte.

The slave writes data directly to LCD memory as it receives it, instead of first storing a full string or bitmap in RAM.

## 6. Reuse scratch variables

The master game loop reuses local variables like `x`, `y`, `i`, `j`, `w`, `h`, `type`, and `r` across different modes.

This keeps the live variable count smaller and helps SDCC fit the program into internal RAM.

## 7. Use LCD graphics RAM for pages

Double-buffering is handled by selecting LCD graphics pages, not by keeping screen buffers inside the MCU.

The MCU only stores object positions/state; the LCD memory stores the pixels.

## 8. Avoid division and heavy math

The code prefers shifts, lookup tables, and simple rejection loops.

Examples:

- `x >> 3` for divide-by-8 text/cell conversion.
- `(i << 4) + (i << 2) + j` for `i * 20 + j`.
- Xorshift RNG instead of a larger random library.

## 9. Keep protocols byte-sized

GPU commands and parameters are sent as bytes. This keeps the bus protocol simple and reduces temporary storage.

## Current Memory Result

From the current build reports:

- Master firmware uses about 7205 bytes of ROM/flash.
- Slave firmware uses about 6470 bytes of ROM/flash.
- Master stack starts at `0x71`.
- Slave stack starts at `0x86`.

The slave has no spare internal RAM reported by the linker, so future features should continue using flash tables, bit packing, and streaming.
