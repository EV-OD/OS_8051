# Setup

This workspace builds 8051 firmware with SDCC and CMake.

## Prerequisites

- `sdcc`
- `sdas8051`
- `sdld`
- `packihx`
- CMake 3.20 or newer

The build also uses these include paths:

- `/home/rabin/.vscode/extensions/kelify.kelify-0.0.7/assets/headers`
- `/usr/share/sdcc/include`

## Project Layout

- `src/` contains master C sources
- `asm/` contains master assembly sources
- `include/` contains shared headers
- `slave/1/` contains the first slave firmware
- more slave folders can be added later under `slave/`

## Configure

```sh
cmake -S . -B build
```

## Build Everything

```sh
cmake --build build
```

This builds both firmware images:

- master: `build/master/master.hex`
- slave 1: `build/slave/1/slave1/slave1.hex`

It also exports the hex files automatically to:

- `/home/rabin/.var/app/com.usebottles.bottles/data/bottles/bottles/Proteus_Lab/drive_c/users/Public/OS_8051_hex/`

Exported filenames:

- `master.hex`
- `slave1.hex`

## Build One Target

```sh
cmake --build build --target master
cmake --build build --target slave1
```

## Notes

- The build uses SDCC MCS-51 small model options.
- C and ASM can call each other through the short bridge symbols used in the sample sources.
- The ASXXXX assembler on this toolchain emits `.rel`, `.lst`, and `.sym` files alongside the source during assembly, then CMake copies them into the build tree before linking.
