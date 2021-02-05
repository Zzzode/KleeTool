# Use the llvm-config binary to get the information needed.
# Try to detect it in the user's environment. The user can force a particular
# binary by passing `-DLLVM_CONFIG_BINARY=/path/to/llvm-config` to CMake.
find_program(LLVM_CONFIG_BINARY NAMES llvm-config)
message(STATUS "LLVM_CONFIG_BINARY: ${LLVM_CONFIG_BINARY}")

if (NOT LLVM_CONFIG_BINARY)
    message(FATAL_ERROR "Failed to find llvm-config.\n"
            "Try passing -DLLVM_CONFIG_BINARY=/path/to/llvm-config to cmake")
endif ()

function(_run_llvm_config output_var)
    set(_command "${LLVM_CONFIG_BINARY}" ${ARGN})
    execute_process(COMMAND ${_command}
            RESULT_VARIABLE _exit_code
            OUTPUT_VARIABLE ${output_var}
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
            )
    if (NOT ("${_exit_code}" EQUAL "0"))
        message(FATAL_ERROR "Failed running ${_command}")
    endif ()
    set(${output_var} ${${output_var}} PARENT_SCOPE)
endfunction()

# Get LLVM version
_run_llvm_config(LLVM_PACKAGE_VERSION "--version")

set(LLVM_DEFINITIONS "")
_run_llvm_config(_llvm_cpp_flags "--cppflags")
string_to_list("${_llvm_cpp_flags}" _llvm_cpp_flags_list)
foreach (flag ${_llvm_cpp_flags_list})
    # Filter out -I flags by only looking for -D flags.
    if ("${flag}" MATCHES "^-D" AND NOT ("${flag}" STREQUAL "-D_DEBUG"))
        list(APPEND LLVM_DEFINITIONS "${flag}")
    endif ()
endforeach ()

set(LLVM_ENABLE_ASSERTIONS ON)
set(LLVM_ENABLE_EH ON)
set(LLVM_ENABLE_RTTI ON)
set(LLVM_ENABLE_VISIBILITY_INLINES_HIDDEN OFF)
_run_llvm_config(_llvm_cxx_flags "--cxxflags")
string_to_list("${_llvm_cxx_flags}" _llvm_cxx_flags_list)
foreach (flag ${_llvm_cxx_flags_list})
    if ("${flag}" STREQUAL "-DNDEBUG")
        # Note we don't rely on `llvm-config --build-mode` because
        # that seems broken when LLVM is built with CMake.
        set(LLVM_ENABLE_ASSERTIONS OFF)
    elseif ("${flag}" STREQUAL "-fno-exceptions")
        set(LLVM_ENABLE_EH OFF)
    elseif ("${flag}" STREQUAL "-fno-rtti")
        set(LLVM_ENABLE_RTTI OFF)
    elseif ("${flag}" STREQUAL "-fvisibility-inlines-hidden")
        set(LLVM_ENABLE_VISIBILITY_INLINES_HIDDEN ON)
    endif ()
endforeach ()

set(LLVM_INCLUDE_DIRS "")
foreach (flag ${_llvm_cpp_flags_list})
    # Filter out -D flags by only looking for -I flags.
    if ("${flag}" MATCHES "^-I")
        string(REGEX REPLACE "^-I(.+)$" "\\1" _include_dir "${flag}")
        list(APPEND LLVM_INCLUDE_DIRS "${_include_dir}")
    endif ()
endforeach ()

_run_llvm_config(LLVM_LIBRARY_DIRS "--libdir")
_run_llvm_config(LLVM_TOOLS_BINARY_DIR "--bindir")
_run_llvm_config(TARGET_TRIPLE "--host-target")
