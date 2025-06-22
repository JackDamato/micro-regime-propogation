# Add both test files to the test executable
add_executable(module1_tests 
    test_dbn_reader.cpp
    test_pipeline.cpp
    bench_dbn_reader.cpp
)

# Link with our module and GTest
target_link_libraries(module1_tests PRIVATE module1 GTest::gtest_main)
target_include_directories(module1_tests PRIVATE ${gtest_SOURCE_DIR}/include)

# Set output directory
set_target_properties(module1_tests 
    PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin"
)

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(module1_tests 
    DISCOVERY_TIMEOUT 10 
    TEST_PREFIX "Module1TestSuite."
    PROPERTIES 
        ENVIRONMENT "TEST_DATA_DIR=${CMAKE_SOURCE_DIR}/data/ES"
)