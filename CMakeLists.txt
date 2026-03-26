cmake_minimum_required(VERSION 3.15)
project(BoostChain VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BOOSTCHAIN_BUILD_TESTS "Build tests" ON)
option(BOOSTCHAIN_BUILD_EXAMPLES "Build examples" ON)
option(BOOSTCHAIN_DISABLE_EXCEPTIONS "Disable exceptions" OFF)

# Library target
add_library(boostchain)
add_library(boostchain::boostchain ALIAS boostchain)

target_include_directories(boostchain
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(boostchain PUBLIC cxx_std_17)

# Dependencies header-only
target_include_directories(boostchain PRIVATE deps)

# Source files
target_sources(boostchain
    PRIVATE
        src/error.cpp
)

# Subdirectories
if(BOOSTCHAIN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BOOSTCHAIN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install
install(TARGETS boostchain
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(DIRECTORY include/boostchain DESTINATION include)
