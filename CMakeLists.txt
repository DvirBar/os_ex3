cmake_minimum_required(VERSION 3.25)
project(hw3_wsl C)

set(CMAKE_C_STANDARD 99)

add_executable(hw3_wsl server.c client.c list.c output.c request.c segel.c)
