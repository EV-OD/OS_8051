include("${CMAKE_CURRENT_LIST_DIR}/include/CMakeLists.txt")
include("${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt")
include("${CMAKE_CURRENT_LIST_DIR}/asm/CMakeLists.txt")

sdcc8051_add_firmware(slave1
    OUTPUT_NAME slave1
    C_SOURCES
        ${OS8051_SLAVE1_C_SOURCES}
    ASM_SOURCES
        ${OS8051_SLAVE1_ASM_SOURCES}
    INCLUDE_DIRS
        ${OS8051_SLAVE1_INCLUDE_DIRS}
    HEADER_DEPENDS
        ${OS8051_SLAVE1_HEADERS}
)
