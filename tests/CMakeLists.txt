find_package(doctest CONFIG REQUIRED)

add_executable(emg_rt_tests
    test_main.cpp

    buffer/test_acquisition_ring_buffer.cpp
    buffer/test_ring_matrix.cpp

    config/test_load_online_decomposer.cpp

    decomposition/test_classify_discharges.cpp
    decomposition/test_get_pulse_train.cpp
    decomposition/test_is_local_max.cpp

    dsp/test_demean.cpp
    dsp/test_extend.cpp

    integration/test_synthetic_pipeline.cpp
)

target_include_directories(emg_rt_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(emg_rt_tests PRIVATE
    emg_rt_lib
    doctest::doctest
)

add_test(NAME emg_rt_tests COMMAND emg_rt_tests)
