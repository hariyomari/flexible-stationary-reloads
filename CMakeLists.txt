#
# CMake configuration
#


# Minimum required CMake version
cmake_minimum_required(VERSION 3.25.0)


# Prevent building inside of the source directory
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Build in a separate directory, e.g.: cmake --preset default")
endif ()


# Project configuration
project(flexible-stationary-reloads VERSION 0.1.0 LANGUAGES C CXX)


# C++17, C++20, C++23, C++26
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# C99, C11, C17, C23
set(CMAKE_C_STANDARD C11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)


#
# Project libraries
#


add_subdirectory(flexible-stationary-reloads)
