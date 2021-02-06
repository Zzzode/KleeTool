#
# Created by xxxx on xxxx/xx/xx.
#
# The user can force a particular binary by passing
# `-DBOOST_ROOT=/path/to/boost` to CMake.
find_program(BOOST_ROOT NAMES boost)
message(STATUS "BOOST_ROOT: ${BOOST_ROOT}")

if (NOT BOOST_ROOT)
    message(FATAL_ERROR "Failed to find Boost.\n"
            "Try passing -DBOOST_ROOT=/path/to/boost to cmake")
endif ()

find_package(Boost COMPONENTS REQUIRED)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Failed to find Boost.\n"
            "Try passing -DBOOST_ROOT=/path/to/boost to cmake")
endif()