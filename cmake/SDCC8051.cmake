function(sdcc8051_add_firmware target_name)
    set(options)
    set(oneValueArgs OUTPUT_NAME)
    set(multiValueArgs C_SOURCES ASM_SOURCES INCLUDE_DIRS HEADER_DEPENDS)
    cmake_parse_arguments(FW "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT FW_OUTPUT_NAME)
        set(FW_OUTPUT_NAME "${target_name}")
    endif()

    set(firmware_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${target_name}")
    file(MAKE_DIRECTORY "${firmware_binary_dir}")

    set(firmware_rel_files)
    set(firmware_depends ${FW_HEADER_DEPENDS})

    set(firmware_include_flags)
    foreach(include_dir IN LISTS FW_INCLUDE_DIRS)
        list(APPEND firmware_include_flags -I "${include_dir}")
    endforeach()

    foreach(source_file IN LISTS FW_C_SOURCES)
        get_filename_component(source_name "${source_file}" NAME_WE)
        set(output_prefix "${firmware_binary_dir}/${source_name}")
        set(output_rel "${output_prefix}.rel")

        add_custom_command(
            OUTPUT "${output_rel}"
            COMMAND "${SDCC_EXECUTABLE}"
                -mmcs51
                --model-small
                --std-sdcc11
                -c
                ${firmware_include_flags}
                "${source_file}"
                -o "${output_rel}"
            DEPENDS "${source_file}" ${firmware_depends}
            COMMENT "Compiling ${source_name}.c for ${target_name}"
            VERBATIM
        )

        list(APPEND firmware_rel_files "${output_rel}")
    endforeach()

    foreach(source_file IN LISTS FW_ASM_SOURCES)
        get_filename_component(source_name "${source_file}" NAME_WE)
        get_filename_component(source_directory "${source_file}" DIRECTORY)
        set(output_prefix "${firmware_binary_dir}/${source_name}")
        set(output_rel "${output_prefix}.rel")
        set(output_lst "${output_prefix}.lst")
        set(output_sym "${output_prefix}.sym")
        set(source_rel "${source_directory}/${source_name}.rel")
        set(source_lst "${source_directory}/${source_name}.lst")
        set(source_sym "${source_directory}/${source_name}.sym")

        add_custom_command(
            OUTPUT "${output_rel}"
            COMMAND "${SDAS8051_EXECUTABLE}"
                -los
                "${source_file}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${source_rel}" "${output_rel}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${source_lst}" "${output_lst}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${source_sym}" "${output_sym}"
            COMMAND "${CMAKE_COMMAND}" -E rm -f "${source_rel}" "${source_lst}" "${source_sym}"
            DEPENDS "${source_file}" ${firmware_depends}
            COMMENT "Assembling ${source_name}.asm for ${target_name}"
            VERBATIM
        )

        list(APPEND firmware_rel_files "${output_rel}")
    endforeach()

    set(linker_file "${firmware_binary_dir}/${FW_OUTPUT_NAME}.lnk")
    set(linker_ihx "${firmware_binary_dir}/${FW_OUTPUT_NAME}.ihx")
    set(linker_hex "${firmware_binary_dir}/${FW_OUTPUT_NAME}.hex")

    get_filename_component(sdcc_bin_dir "${SDCC_EXECUTABLE}" DIRECTORY)
    set(sdcc_library_candidates
        "${sdcc_bin_dir}/../share/sdcc/lib/small"
        "/usr/share/sdcc/lib/small"
    )

    set(sdcc_library_paths)
    foreach(candidate_dir IN LISTS sdcc_library_candidates)
        if(EXISTS "${candidate_dir}")
            list(APPEND sdcc_library_paths "${candidate_dir}")
        endif()
    endforeach()

    if(sdcc_library_paths STREQUAL "")
        message(FATAL_ERROR "Could not locate the SDCC small-model library directory")
    endif()

    set(linker_library_path_lines)
    foreach(candidate_dir IN LISTS sdcc_library_paths)
        list(APPEND linker_library_path_lines "-k ${candidate_dir}")
    endforeach()

    set(linker_library_file_lines
        "-l mcs51"
        "-l libsdcc"
        "-l libint"
        "-l liblong"
        "-l libfloat"
    )

    string(REPLACE ";" "\n" linker_input_lines "${firmware_rel_files}")
    string(REPLACE ";" "\n" linker_library_path_text "${linker_library_path_lines}")
    string(REPLACE ";" "\n" linker_library_file_text "${linker_library_file_lines}")

    set(LINKER_IHX_FILE "${linker_ihx}")
    set(LINKER_LIBRARY_PATHS "${linker_library_path_text}")
    set(LINKER_LIBRARY_FILES "${linker_library_file_text}")
    set(LINKER_INPUT_FILES "${linker_input_lines}")

    configure_file(
        "${PROJECT_SOURCE_DIR}/linker/8051.lnk.in"
        "${linker_file}"
        @ONLY
    )

    add_custom_command(
        OUTPUT "${linker_ihx}"
        COMMAND "${SDLD_EXECUTABLE}" -nf "${linker_file}"
        DEPENDS ${firmware_rel_files} "${linker_file}"
        WORKING_DIRECTORY "${firmware_binary_dir}"
        COMMENT "Linking ${target_name} to Intel HEX"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${linker_hex}"
        COMMAND /bin/sh -c "\"${PACKIHX_EXECUTABLE}\" \"$1\" > \"$2\"" _ "${linker_ihx}" "${linker_hex}"
        DEPENDS "${linker_ihx}"
        WORKING_DIRECTORY "${firmware_binary_dir}"
        COMMENT "Packing ${target_name} to final HEX"
        VERBATIM
    )

    set_property(GLOBAL APPEND PROPERTY OS8051_HEX_FILES "${linker_hex}")
    set_property(GLOBAL APPEND PROPERTY OS8051_FIRMWARE_TARGETS "${target_name}")

    add_custom_target(${target_name} ALL DEPENDS "${linker_hex}")
endfunction()
