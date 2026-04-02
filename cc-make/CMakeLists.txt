cmake_minimum_required(VERSION 3.21)
project(cc-make VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add Homebrew prefix paths for cmake config packages
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")

find_package(nlohmann_json CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

# Find PCRE2 and CURL via find_library (Homebrew doesn't provide CMake configs)
find_path(PCRE2_INCLUDE_DIRS NAMES pcre2.h)
find_library(PCRE2_LIBRARY NAMES pcre2-8 pcre2)
find_path(CURL_INCLUDE_DIRS NAMES curl/curl.h)
find_library(CURL_LIBRARY NAMES curl)

# Create imported targets for PCRE2 and CURL
add_library(PCRE2::PCRE2 UNKNOWN IMPORTED)
set_target_properties(PCRE2::PCRE2 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PCRE2_INCLUDE_DIRS}"
    IMPORTED_LOCATION "${PCRE2_LIBRARY}"
)

add_library(CURL::libcurl UNKNOWN IMPORTED)
set_target_properties(CURL::libcurl PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}"
    IMPORTED_LOCATION "${CURL_LIBRARY}"
)

# Main library (header-only where possible, compiled where needed)
# Gathers all source files except main.cpp (which belongs to the executable)
file(GLOB_RECURSE CC_SOURCES src/*.cpp)
list(REMOVE_ITEM CC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
add_library(cc-make-lib STATIC ${CC_SOURCES})
target_include_directories(cc-make-lib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(cc-make-lib PUBLIC
    nlohmann_json::nlohmann_json
    CLI11::CLI11
    spdlog::spdlog
    fmt::fmt
    PCRE2::PCRE2
    CURL::libcurl
    yaml-cpp::yaml-cpp
    Boost::boost Boost::system Boost::filesystem
)

# Main executable
add_executable(cc-make src/main.cpp)
target_link_libraries(cc-make PRIVATE cc-make-lib)

# Tests
enable_testing()
add_subdirectory(tests)
