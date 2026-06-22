cmake_minimum_required(VERSION 3.16)
project(hermes-pi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Common protocol header lives one level up
set(HERMES_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common")

add_executable(hermes-pi
    src/main.cpp
    src/udp_server.cpp
    src/serial_bridge.cpp
)

target_include_directories(hermes-pi PRIVATE
    include
    ${HERMES_COMMON_DIR}
)

option(HERMES_DEBUG "Enable debug trace prints" OFF)
if(HERMES_DEBUG)
    target_compile_definitions(hermes-pi PRIVATE HERMES_DEBUG)
endif()

target_compile_options(hermes-pi PRIVATE -Wall -Wextra -Wpedantic)

# No external dependencies — POSIX sockets + termios are part of libc
