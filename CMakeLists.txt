cmake_minimum_required(VERSION 3.20)
project(emg-rt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(yaml-cpp REQUIRED)

add_executable(emg-rt
    src/main.cpp

    src/config/decomposition_config.cpp

    src/decomposition/get_discharge_t.cpp
    src/decomposition/get_pulse_train.cpp
    src/decomposition/is_local_max.cpp

    src/dsp/demean.cpp
    src/dsp/extend.cpp
)

target_include_directories(emg-rt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(emg-rt PRIVATE 
    yaml-cpp::yaml-cpp
)

target_compile_options(emg-rt PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
)
