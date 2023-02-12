cmake_minimum_required(VERSION 3.21)
project(MPI C)

find_package(MPI REQUIRED)

set(CMAKE_C_STANDARD 99)

add_executable(MPI main.c)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")

target_link_libraries(MPI PUBLIC MPI::MPI_C)