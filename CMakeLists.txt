cmake_minimum_required(VERSION 3.24)
project(os_ex3 C)

set(CMAKE_C_STANDARD 99)

add_executable(os_ex3 client.c output.c request.c segel.c server.c)
