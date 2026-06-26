cmake_minimum_required(VERSION 3.20)
project(emg-rt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(yaml-cpp REQUIRED)
find_package(doctest REQUIRED)

enable_testing()

add_library(emg_rt_lib
    src/buffer/signal_ring_buffer.cpp

    src/config/decomposition_config.cpp

    src/data_replay/offline_emg_source.cpp

    src/decomposition/get_discharge_t.cpp
    src/decomposition/get_pulse_train.cpp
    src/decomposition/is_local_max.cpp
    src/decomposition/online_decomposer.cpp

    src/dsp/demean.cpp
    src/dsp/extend.cpp
)

target_include_directories(emg_rt_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(emg_rt_lib PRIVATE
    yaml-cpp::yaml-cpp
)

target_compile_options(emg_rt_lib PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
)

add_executable(emg-rt
    src/main.cpp
)

target_link_libraries(emg-rt PRIVATE
    emg_rt_lib
)

target_compile_options(emg-rt PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
)

add_subdirectory(tests)
