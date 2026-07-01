find_package(doctest CONFIG REQUIRED)

add_executable(emg_rt_tests
    test_main.cpp

    decomposition/test_get_distime.cpp
    decomposition/test_get_pulse_train.cpp
    decomposition/test_incremental_is_local_max.cpp

    dsp/test_demean.cpp
    dsp/test_incremental_extend.cpp

    utils/test_ring_matrix.cpp
)

target_include_directories(emg_rt_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(emg_rt_tests PRIVATE
    emg_rt_lib
    doctest::doctest
)

add_test(NAME emg_rt_tests COMMAND emg_rt_tests)
