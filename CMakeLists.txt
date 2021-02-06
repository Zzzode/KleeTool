#
# Created by xxxx on xxxx/xx/xx.
#

#######################################################
# Minimum CMake version and policies
#######################################################
cmake_minimum_required(VERSION 3.16)
project(KleeTool CXX C)

#######################################################
# C++ version
#######################################################
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#######################################################
# Project version
#######################################################
set(KleeTool_VERSION_MAJOR 1)
set(KleeTool_VERSION_MINOR 0)
set(KleeTool_VERSION "${KleeTool_VERSION_MAJOR}.${KleeTool_VERSION_MINOR}")
message(STATUS "KleeTool version ${KleeTool_VERSION}")

#######################################################
# Include useful CMake functions
#######################################################
include("${CMAKE_SOURCE_DIR}/cmake/string_to_list.cmake")

#######################################################
# Find pThread
#######################################################
find_package(Threads)

#######################################################
# Find Boost
#######################################################
include(${CMAKE_SOURCE_DIR}/cmake/find_boost.cmake)
set(NEEDED_BOOST_VARS
        Boost_INCLUDE_DIRS
        Boost_LIBRARY_DIRS
        Boost_VERSION
        Boost_LIB_VERSION)

foreach (vname ${NEEDED_BOOST_VARS})
    message(STATUS "${vname}: \"${${vname}}\"")
    if (NOT (DEFINED "${vname}"))
        message(FATAL_ERROR "${vname} was not defined")
    endif ()
endforeach ()

#######################################################
# Find LLVM
#######################################################
include(${CMAKE_SOURCE_DIR}/cmake/find_llvm.cmake)
set(NEEDED_LLVM_VARS
        LLVM_PACKAGE_VERSION
        LLVM_DEFINITIONS
        LLVM_ENABLE_ASSERTIONS
        LLVM_ENABLE_EH
        LLVM_ENABLE_RTTI
        LLVM_INCLUDE_DIRS
        LLVM_LIBRARY_DIRS
        LLVM_TOOLS_BINARY_DIR
        # LLVM_ENABLE_VISIBILITY_INLINES_HIDDEN
        TARGET_TRIPLE)

foreach (vname ${NEEDED_LLVM_VARS})
    message(STATUS "${vname}: \"${${vname}}\"")
    if (NOT (DEFINED "${vname}"))
        message(FATAL_ERROR "${vname} was not defined")
    endif ()
endforeach ()

llvm_map_components_to_libnames(LLVM_LIBS support)

#######################################################
# Other opts
#######################################################
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

link_directories(${PROJECT_SOURCE_DIR}/lib)
include_directories(${PROJECT_SOURCE_DIR}/include)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_FILES)

add_executable(KleeTool ${SRC_FILES})
target_link_libraries(KleeTool PUBLIC ${LLVM_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})