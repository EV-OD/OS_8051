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

set(OS8051_EXPORT_DIR "/home/rabin/.var/app/com.usebottles.bottles/data/bottles/bottles/Proteus_Lab/drive_c/users/Public/OS_8051_hex")

get_property(OS8051_HEX_FILES GLOBAL PROPERTY OS8051_HEX_FILES)
get_property(OS8051_FIRMWARE_TARGETS GLOBAL PROPERTY OS8051_FIRMWARE_TARGETS)

if(NOT OS8051_HEX_FILES)
    message(FATAL_ERROR "No firmware hex files were registered for export")
endif()

if(NOT OS8051_FIRMWARE_TARGETS)
    message(FATAL_ERROR "No firmware targets were registered for export")
endif()

set(OS8051_EXPORT_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/ExportHex.cmake")

set(EXPORT_DIRECTORY "${OS8051_EXPORT_DIR}")
set(EXPORT_HEX_FILES "${OS8051_HEX_FILES}")

configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/ExportHex.cmake.in"
    "${OS8051_EXPORT_SCRIPT}"
    @ONLY
)

add_custom_target(export_hex ALL
    DEPENDS ${OS8051_HEX_FILES}
    COMMAND "${CMAKE_COMMAND}" -P "${OS8051_EXPORT_SCRIPT}"
    COMMENT "Exporting firmware hex files to Proteus"
    VERBATIM
)

add_dependencies(export_hex ${OS8051_FIRMWARE_TARGETS})
