cmake_minimum_required(VERSION 3.20)

project(OS_8051 LANGUAGES NONE)

find_program(SDCC_EXECUTABLE sdcc REQUIRED)
find_program(SDAS8051_EXECUTABLE sdas8051 REQUIRED)
find_program(SDLD_EXECUTABLE sdld REQUIRED)
find_program(PACKIHX_EXECUTABLE packihx REQUIRED)

set(OS8051_SDCC_INCLUDE_DIRS
    "/home/rabin/.vscode/extensions/kelify.kelify-0.0.7/assets/headers"
    "/usr/share/sdcc/include"
)

include("${PROJECT_SOURCE_DIR}/cmake/SDCC8051.cmake")

include("${PROJECT_SOURCE_DIR}/include/CMakeLists.txt")
include("${PROJECT_SOURCE_DIR}/src/CMakeLists.txt")
include("${PROJECT_SOURCE_DIR}/asm/CMakeLists.txt")

sdcc8051_add_firmware(master
    OUTPUT_NAME master
    C_SOURCES
        ${OS8051_MASTER_C_SOURCES}
    ASM_SOURCES
        ${OS8051_MASTER_ASM_SOURCES}
    INCLUDE_DIRS
        ${OS8051_MASTER_INCLUDE_DIRS}
    HEADER_DEPENDS
        ${OS8051_MASTER_HEADERS}
)

add_subdirectory(slave)
