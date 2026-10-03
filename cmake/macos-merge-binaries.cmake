# Merging two directories (arm64 and x64) into a single directory with fat binaries on macOS
cmake_minimum_required(VERSION 3.22)

# Check if running on macOS
if (NOT APPLE)
    message(FATAL_ERROR "This script is designed to run on macOS only.")
endif ()

# Validate input parameters
if (NOT DEFINED DIR_ARM64
    OR NOT DEFINED DIR_X64
    OR NOT DEFINED DIR_OUT)
    message(FATAL_ERROR "DIR_ARM64, DIR_X64, and DIR_OUT must be defined.")
endif ()

# Convert paths to absolute
get_filename_component(ABS_DIR_ARM64 "${DIR_ARM64}" ABSOLUTE)
get_filename_component(ABS_DIR_X64 "${DIR_X64}" ABSOLUTE)
get_filename_component(ABS_DIR_OUT "${DIR_OUT}" ABSOLUTE)

# Ensure input directories exist
if (NOT EXISTS "${ABS_DIR_ARM64}")
    message(FATAL_ERROR "arm64 directory does not exist: ${ABS_DIR_ARM64}")
endif ()
if (NOT EXISTS "${ABS_DIR_X64}")
    message(FATAL_ERROR "x64 directory does not exist: ${ABS_DIR_X64}")
endif ()

# Find lipo tool
find_program(LIPO lipo REQUIRED)
if (NOT LIPO)
    message(FATAL_ERROR "lipo tool not found. Ensure Xcode is installed.")
endif ()

# Find diff tool
find_program(DIFF diff REQUIRED)
if (NOT DIFF)
    message(FATAL_ERROR "diff tool not found.")
endif ()

# Function to check if a file is a Mach-O binary or static library
function (is_macho_or_static_lib file_path result_var)
    execute_process(
        COMMAND file "${file_path}"
        OUTPUT_VARIABLE file_output
        RESULT_VARIABLE file_result)
    if (file_result EQUAL 0)
        if (file_output MATCHES "Mach-O" OR file_output MATCHES "ar archive")
            set(${result_var}
                TRUE
                PARENT_SCOPE)
        else ()
            set(${result_var}
                FALSE
                PARENT_SCOPE)
        endif ()
    else ()
        set(${result_var}
            FALSE
            PARENT_SCOPE)
    endif ()
endfunction ()

# Function to compare two files for equality
function (compare_files file1 file2 are_equal_var)
    execute_process(COMMAND cmp -s "${file1}" "${file2}"
                    RESULT_VARIABLE cmp_result)
    if (cmp_result EQUAL 0)
        set(${are_equal_var}
            TRUE
            PARENT_SCOPE)
    else ()
        # Run diff to get differences
        execute_process(
            COMMAND "${DIFF}" "${file1}" "${file2}"
            OUTPUT_VARIABLE diff_output
            RESULT_VARIABLE diff_result)
        set(${are_equal_var}
            FALSE
            PARENT_SCOPE)
        # Store diff output in parent scope variable for error message
        set(DIFF_OUTPUT
            "${diff_output}"
            PARENT_SCOPE)
    endif ()
endfunction ()

# Function to check if a file matches a preference list
function (file_matches_preference file_path pref_list result_var)
    set(matches FALSE)
    foreach (pref IN LISTS pref_list)
        if ("${file_path}" MATCHES "${pref}")
            set(matches TRUE)
            break()
        endif ()
    endforeach ()
    set(${result_var}
        ${matches}
        PARENT_SCOPE)
endfunction ()

# Function to merge a single file pair using lipo
function (merge_file arm64_file x64_file output_file)
    # Ensure both input files exist
    if (NOT EXISTS "${arm64_file}")
        message(FATAL_ERROR "arm64 file does not exist: ${arm64_file}")
    endif ()
    if (NOT EXISTS "${x64_file}")
        message(FATAL_ERROR "x64 file does not exist: ${x64_file}")
    endif ()

    # Check if files are Mach-O or static libraries
    is_macho_or_static_lib("${arm64_file}" is_arm64_macho)
    is_macho_or_static_lib("${x64_file}" is_x64_macho)

    # Ensure both files are either Mach-O/static lib or neither
    if (is_arm64_macho AND NOT is_x64_macho)
        message(
            FATAL_ERROR
                "File type mismatch: ${arm64_file} is Mach-O/static lib, but ${x64_file} is not"
        )
    endif ()
    if (NOT is_arm64_macho AND is_x64_macho)
        message(
            FATAL_ERROR
                "File type mismatch: ${x64_file} is Mach-O/static lib, but ${arm64_file} is not"
        )
    endif ()

    if (is_arm64_macho AND is_x64_macho)
        # Run lipo to create fat binary
        execute_process(
            COMMAND "${LIPO}" -create "${arm64_file}" "${x64_file}" -output
                    "${output_file}" RESULT_VARIABLE lipo_result)
        if (NOT lipo_result EQUAL 0)
            message(
                FATAL_ERROR
                    "Failed to merge ${arm64_file} and ${x64_file} with lipo")
        endif ()
        message(
            STATUS "Merged ${arm64_file} and ${x64_file} into ${output_file}")
    else ()
        # Check for preference lists
        file_matches_preference("${arm64_file}" "${PREFER_X64}" prefer_x64)
        file_matches_preference("${arm64_file}" "${PREFER_ARM64}" prefer_arm64)

        if (prefer_x64 AND prefer_arm64)
            message(
                FATAL_ERROR
                    "File ${arm64_file} matches both PREFER_X64 and PREFER_ARM64 lists"
            )
        elseif (prefer_x64)
            file(COPY_FILE "${x64_file}" "${output_file}")
            message(
                STATUS "Copied ${x64_file} to ${output_file} (PREFER_X64 match)"
            )
        elseif (prefer_arm64)
            file(COPY_FILE "${arm64_file}" "${output_file}")
            message(
                STATUS
                    "Copied ${arm64_file} to ${output_file} (PREFER_ARM64 match)"
            )
        else ()
            # Compare files if not Mach-O or static library and no preference
            compare_files("${arm64_file}" "${x64_file}" files_equal)
            if (NOT files_equal)
                message(
                    FATAL_ERROR
                        "Non-Mach-O files differ: ${arm64_file} and ${x64_file}\nDiff output:\n${DIFF_OUTPUT}"
                )
            endif ()
            # Copy x64 file
            file(COPY_FILE "${x64_file}" "${output_file}")
            message(
                STATUS
                    "Copied ${x64_file} to ${output_file} (non-Mach-O or non-static library)"
            )
        endif ()
    endif ()
endfunction ()

# Function to recursively process directories
function (process_directory arm64_base x64_base output_base)
    # Create output base directory
    file(MAKE_DIRECTORY "${output_base}")

    # Get all files and subdirectories in arm64 directory
    file(
        GLOB_RECURSE arm64_items
        RELATIVE "${arm64_base}"
        "${arm64_base}/*")
    # Get all files and subdirectories in x64 directory
    file(
        GLOB_RECURSE x64_items
        RELATIVE "${x64_base}"
        "${x64_base}/*")
    list(FILTER arm64_items EXCLUDE REGEX "^share/vcpkg-")
    list(FILTER x64_items EXCLUDE REGEX "^share/vcpkg-")

    # Ensure the set of files is identical
    list(SORT arm64_items)
    list(SORT x64_items)
    if (NOT "${arm64_items}" STREQUAL "${x64_items}")
        # Write file lists to temporary files
        set(temp_arm64_list "${CMAKE_BINARY_DIR}/arm64_files.txt")
        set(temp_x64_list "${CMAKE_BINARY_DIR}/x64_files.txt")
        file(WRITE "${temp_arm64_list}" "")
        file(WRITE "${temp_x64_list}" "")
        foreach (item ${arm64_items})
            file(APPEND "${temp_arm64_list}" "${item}\n")
        endforeach ()
        foreach (item ${x64_items})
            file(APPEND "${temp_x64_list}" "${item}\n")
        endforeach ()

        # Run diff to get differences
        execute_process(
            COMMAND "${DIFF}" "${temp_arm64_list}" "${temp_x64_list}"
            OUTPUT_VARIABLE diff_output
            RESULT_VARIABLE diff_result)

        # Clean up temporary files
        file(REMOVE "${temp_arm64_list}" "${temp_x64_list}")

        message(
            FATAL_ERROR
                "Directories have different sets of files. Diff output:\n${diff_output}"
        )
    endif ()

    foreach (item ${arm64_items})
        set(arm64_path "${arm64_base}/${item}")
        set(x64_path "${x64_base}/${item}")
        set(output_path "${output_base}/${item}")

        get_filename_component(output_dir "${output_path}" DIRECTORY)
        file(MAKE_DIRECTORY "${output_dir}")

        merge_file("${arm64_path}" "${x64_path}" "${output_path}")
    endforeach ()
endfunction ()

# Create output directory
file(MAKE_DIRECTORY "${ABS_DIR_OUT}")

process_directory("${ABS_DIR_ARM64}" "${ABS_DIR_X64}" "${ABS_DIR_OUT}")
