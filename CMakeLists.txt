cmake_minimum_required(VERSION 3.15)
project(TinyServer)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_BUILD_TYPE RELEASE)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 ")

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/timer)

#include_directories(${PROJECT_SOURCE_DIR})

add_executable(TinyServer main.cpp threadpool.cpp models/http_conn.cpp include/utils.h include/common_include.h models/MyProactor.cpp models/MyProactor.h)
target_link_libraries(TinyServer pthread)