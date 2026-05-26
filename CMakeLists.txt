cmake_minimum_required(VERSION 3.20)
project(emg-rt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(emg-rt
    src/main.cpp
)

target_include_directories(emg-rt PRIVATE include)

target_compile_options(emg-rt PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
)
