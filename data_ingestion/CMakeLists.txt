cmake_minimum_required(VERSION 3.24)
project(data_ingestion LANGUAGES CXX)

add_library(data_ingestion
    src/core/event_parser.cpp
    src/core/feature_engine.cpp
    src/core/order_book.cpp
    src/core/order_engine.cpp
    src/data/dbn_reader.cpp
    src/utils/logger.cpp
    src/utils/stats.cpp
    src/utils/timer.cpp
)

target_include_directories(data_ingestion PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Link against databento only for module1
target_link_libraries(data_ingestion PUBLIC databento::databento)

target_compile_features(data_ingestion PUBLIC cxx_std_20)

# Add tests if enabled
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.14.0)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()