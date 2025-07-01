cmake_minimum_required(VERSION 3.24)
project(feature_generation LANGUAGES CXX)

add_library(feature_generation
    src/core/feature_processor.cpp
    src/core/feature_normalizer.cpp
    src/core/timestamp_pipeline.cpp
    src/data/features_to_csv.cpp
)

target_include_directories(feature_generation PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(feature_generation PUBLIC data_ingestion)

target_compile_features(feature_generation PUBLIC cxx_std_20)

add_executable(features_to_csv src/data/features_to_csv.cpp)
target_link_libraries(features_to_csv PUBLIC feature_generation)

# # Add tests if enabled
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.14.0)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()