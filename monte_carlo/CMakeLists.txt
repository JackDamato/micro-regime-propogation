cmake_minimum_required(VERSION 3.24)
project(monte_carlo LANGUAGES CXX CUDA)

find_package(CUDAToolkit REQUIRED)
enable_language(CUDA)
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -std=c++20")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Use absolute paths to avoid confusion
set(CUDA_KERNELS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/cuda/MonteCarloKernels.cu")
set(CUDA_KERNELS_OBJ "${CMAKE_CURRENT_BINARY_DIR}/MonteCarloKernels.obj")

# Create custom build command with corrected include paths
add_custom_command(
  OUTPUT ${CUDA_KERNELS_OBJ}
  COMMAND ${CUDAToolkit_NVCC_EXECUTABLE}
          --generate-code=arch=compute_89,code=sm_89
          -std=c++20
          -c
          -I "${CMAKE_CURRENT_SOURCE_DIR}/include"          # Main include dir
          -I "${CMAKE_CURRENT_SOURCE_DIR}/include/core"     # For TensorXD.hpp
          -I "${CMAKE_SOURCE_DIR}/feature_generation/include"
          -I "${CMAKE_SOURCE_DIR}/data_ingestion/include"
          -I "${CMAKE_SOURCE_DIR}/databento-cpp/include"
          -I "${CUDAToolkit_INCLUDE_DIRS}"
          -Xcompiler "/EHsc"
          -Xcompiler "/W3"
          -Xcompiler "/nologo"
          -Xcompiler "/Od"
          -Xcompiler "/Zi"
          -Xcompiler "/RTC1"
          -Xcompiler "/MDd"
          -Xcompiler "/wd4819"
          -o "${CUDA_KERNELS_OBJ}"
          "${CUDA_KERNELS_SRC}"
  DEPENDS "${CUDA_KERNELS_SRC}"
  COMMENT "Compiling CUDA kernels with custom command"
  VERBATIM
)

# Main library
add_library(monte_carlo
    src/MonteCarloEngine.cpp
    src/MonteCarloResults.cpp
    src/RegimeParameterMap.cpp
    src/DeviceBuffers.cpp
    ${CUDA_KERNELS_OBJ}  # Use pre-compiled object
)

# Include directories for the whole target
target_include_directories(monte_carlo PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/core>  # For TensorXD.hpp
)

target_link_libraries(monte_carlo PUBLIC 
    feature_generation
    CUDA::cudart
)

target_compile_features(monte_carlo PUBLIC cxx_std_20)

# MSVC specific options
if(MSVC)
    target_compile_options(monte_carlo PRIVATE /wd4819)
endif()

# Add tests if enabled
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(googletest 
        GIT_REPOSITORY https://github.com/google/googletest.git 
        GIT_TAG v1.14.0)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()