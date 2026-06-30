cmake_minimum_required(VERSION 3.20)

project(emg-rt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)

find_package(yaml-cpp REQUIRED)

if(TARGET yaml-cpp::yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp::yaml-cpp)
elseif(TARGET yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp)
else()
    message(FATAL_ERROR "yaml-cpp found, but no usable CMake target was exported")
endif()

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
    ${YAML_CPP_TARGET}
    Eigen3::Eigen
)

target_compile_options(emg_rt_lib PRIVATE -mcpu=native
    -O3
    -DNDEBUG
    -DEIGEN_NO_DEBUG
    -fno-math-errno
    -fno-trapping-math
)

target_compile_definitions(emg_rt_lib PRIVATE EMG_RT_ENABLE_PROFILING)

add_executable(emg-rt
    src/main.cpp
)

target_link_libraries(emg-rt PRIVATE
    emg_rt_lib
)
