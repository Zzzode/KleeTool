cmake_minimum_required(VERSION 3.16)
project(KleeTool)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads)
find_package(LLVM REQUIRED CONFIG)

#设定可执行二进制文件的目录
SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#设定存放编译出来的库文件的目录
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#并且把该目录设为连接目录
LINK_DIRECTORIES( ${PROJECT_SOURCE_DIR}/lib)

#设定头文件目录
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/include)

#增加子文件夹(注意我们的src路径是通过子文件夹的形式添加的，在这里面也是可以些CMakeLists.txt的)
#ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/src)
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/src SRC_FILES)
#SET(SRC_LIST ${PROJECT_SOURCE_DIR}/src)

ADD_EXECUTABLE(KleeTool ${SRC_FILES})
