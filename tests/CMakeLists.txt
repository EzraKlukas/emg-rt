add_executable(emg_rt_tests
    test_main.cpp
    test_dsp.cpp
    test_decomposition.cpp
    test_types.cpp
    test_config.cpp
)

target_link_libraries(emg_rt_tests PRIVATE
    emg_rt_lib
    doctest::doctest
)

target_compile_options(emg_rt_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
)

target_compile_definitions(emg_rt_tests PRIVATE
    TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

add_test(NAME emg_rt_tests COMMAND emg_rt_tests)
