cmake_minimum_required(VERSION 2.8.5)

enable_testing()

set(CMAKE_C_FLAGS "-Wall -O2 -g -std=gnu99 -D_XOPEN_SOURCE=700 -D__EXTENSIONS__ -D_REENTRANT")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-inline-small-functions")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-inline-functions-called-once")

add_subdirectory(src)
