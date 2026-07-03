cmake_minimum_required(VERSION 3.20)

project(emg-rt LANGUAGES CXX)

# -----------------------------------------------------------------------------
# Global project settings
# -----------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# Build options
# -----------------------------------------------------------------------------

option(EMG_RT_ENABLE_PROFILING "Enable scoped profiling timers" OFF)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(Eigen3 REQUIRED)
find_package(yaml-cpp REQUIRED)

if(TARGET yaml-cpp::yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp::yaml-cpp)
elseif(TARGET yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp)
else()
    message(FATAL_ERROR "yaml-cpp found, but no usable CMake target was exported")
endif()

# -----------------------------------------------------------------------------
# Library: emg_rt_lib
# -----------------------------------------------------------------------------

add_library(emg_rt_lib
    src/buffer/acquisition_ring_buffer.cpp

    src/config/decomposition_config.cpp

    src/data_replay/offline_emg_source.cpp

    src/decomposition/classify_discharges.cpp
    src/decomposition/get_pulse_train.cpp
    src/decomposition/is_local_max.cpp
    src/decomposition/online_decomposer.cpp

    src/dsp/demean.cpp
    src/dsp/extend.cpp
)

target_include_directories(emg_rt_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(emg_rt_lib
    PUBLIC
        Eigen3::Eigen

    PRIVATE
        ${YAML_CPP_TARGET}
)

target_compile_options(emg_rt_lib
    PRIVATE
        -mtune=native
        -g
        -O3
        -fno-math-errno
        -fno-trapping-math
)

target_compile_definitions(emg_rt_lib
    PUBLIC
        $<$<BOOL:${EMG_RT_ENABLE_PROFILING}>:EMG_RT_ENABLE_PROFILING>

    PRIVATE
        NDEBUG
        EIGEN_NO_DEBUG
)

# -----------------------------------------------------------------------------
# Executable: emg-rt
# -----------------------------------------------------------------------------

add_executable(emg-rt
    src/main.cpp
)

target_link_libraries(emg-rt
    PRIVATE
        emg_rt_lib
)

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

# enable_testing()
# add_subdirectory(tests)
