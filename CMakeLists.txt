cmake_minimum_required(VERSION 3.18)

project("foray-examples")

# Include Compiler Config (sets c++ 20 and compiler flags)
include("foray/cmakescripts/compilerconfig.cmake")

# Add cmake script dir to cmake's ""Path""
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

add_subdirectory("foray")

include(foray_example)

# Add subdirectories
add_subdirectory("examples")
