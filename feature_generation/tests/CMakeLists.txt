# Add both test files to the test executable
add_executable(module2_tests 
    test_feature_processor.cpp
    test_timestamp_pipeline.cpp
)

# Link with our module and GTest
target_link_libraries(module2_tests PRIVATE feature_generation GTest::gtest_main)
target_include_directories(module2_tests PRIVATE ${gtest_SOURCE_DIR}/include)

# Set output directory
set_target_properties(module2_tests 
    PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin"
)

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(module2_tests 
    DISCOVERY_TIMEOUT 10 
    TEST_PREFIX "Module2TestSuite."
    PROPERTIES 
        ENVIRONMENT "TEST_DATA_DIR=${CMAKE_SOURCE_DIR}/data/ES"
)