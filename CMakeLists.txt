cmake_minimum_required(VERSION 3.24)
project(MicroRegimeProject LANGUAGES CXX)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.43.34808/bin/Hostx64/x64/cl.exe")
set(CMAKE_TOOLCHAIN_FILE "C:/Users/jackd/OneDrive/Documents/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Force Serialize (to avoid pdb C1090 error)
if (MSVC)
    add_compile_options(/FS)
endif()


# Make a temp object that basically overrides databento's check, since the file exists we just don't have it explicitly defined for databento
add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED GLOBAL)
set_target_properties(nlohmann_json::nlohmann_json PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "C:/Users/jackd/OneDrive/Documents/vcpkg/installed/x64-windows/include")
message(STATUS "Manually defined nlohmann_json::nlohmann_json as an imported interface target.")

add_library(httplib::httplib INTERFACE IMPORTED GLOBAL)
set_target_properties(httplib::httplib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "C:/Users/jackd/OneDrive/Documents/vcpkg/installed/x64-windows/include")
message(STATUS "Manually defined httplib::httplib as an imported interface target.")

add_library(date::date INTERFACE IMPORTED GLOBAL)
set_target_properties(date::date PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "C:/Users/jackd/OneDrive/Documents/vcpkg/installed/x64-windows/include")
message(STATUS "Manually defined date::date as an imported interface target.")

# Databento has these 
set(DATABENTO_USE_EXTERNAL_DATE ON CACHE BOOL "" FORCE)
set(DATABENTO_USE_EXTERNAL_JSON ON CACHE BOOL "" FORCE)
set(DATABENTO_USE_EXTERNAL_HTTPLIB ON CACHE BOOL "" FORCE)

# Add databento-cpp first, so its target is globally visible
add_subdirectory(databento-cpp)

# Solves error C1090: PDB API call failed, error code '3'
if(MSVC)
    set_target_properties(databento PROPERTIES
        COMPILE_PDB_NAME_DEBUG             "databento"
        COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/pdb"
        COMPILE_PDB_NAME_RELWITHDEBINFO    "databento"
        COMPILE_PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/pdb"
    )
endif()


# Add my modules
add_subdirectory(data_ingestion)
add_subdirectory(feature_generation)