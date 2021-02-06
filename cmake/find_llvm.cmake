#
# Created by xxxx on xxxx/xx/xx.
#
# Use the llvm-config binary to get the information needed.
# Try to detect it in the user's environment. The user can force a particular
# binary by passing `-DLLVM_CONFIG_BINARY=/path/to/llvm-config` to CMake.
option(USE_CMAKE_FIND_PACKAGE_LLVM "Use find_package(LLVM CONFIG) to find LLVM" OFF)

find_program(LLVM_CONFIG_BINARY NAMES llvm-config)
message(STATUS "LLVM_CONFIG_BINARY: ${LLVM_CONFIG_BINARY}")

if (NOT LLVM_CONFIG_BINARY)
    message(FATAL_ERROR "Failed to find llvm-config.\n"
            "Try passing -DLLVM_CONFIG_BINARY=/path/to/llvm-config to cmake")
endif ()

find_package(LLVM REQUIRED CONFIG)

if(LLVM_FOUND)
    include_directories(${LLVM_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Failed to find Boost.\n"
            "Try passing -DLLVM_CONFIG_BINARY=/path/to/llvm-config to cmake")
endif()

