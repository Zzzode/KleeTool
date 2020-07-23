cmake_minimum_required(VERSION 3.16)
project(KleeTool)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads)
find_package(LLVM REQUIRED CONFIG)

add_executable(KleeTool main.cpp ModifyLLVM.cpp ModifyLLVM.h SearchC.cpp SearchC.h Controller.cpp Controller.h)
target_link_libraries(KleeTool ${CMAKE_THREAD_LIBS_INIT})