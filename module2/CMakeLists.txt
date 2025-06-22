cmake_minimum_required(VERSION 3.24)
project(module2 LANGUAGES CXX)

add_library(module2
    src/core/feature_processor.cpp
    src/core/feature_normalizer.cpp
)

target_include_directories(module2 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(module2 PUBLIC module1)

target_compile_features(module2 PUBLIC cxx_std_20)

# # Add tests if enabled
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.14.0)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()