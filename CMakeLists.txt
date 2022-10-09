cmake_minimum_required(VERSION 3.18)

project("foray-intel-sponza")

enable_language(CXX)
enable_language(CUDA)
set(CMAKE_CUDA_STANDARD 20)

enable_language(CXX)
enable_language(CUDA)
set(CMAKE_CUDA_STANDARD 20)

# Include Compiler Config (sets c++ 20 and compiler flags)
include("foray/cmakescripts/compilerconfig.cmake")

# Run Locate SDL2 script
set(SDL2_HINT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/hsk_rt_rpf/third_party")
include("foray/cmakescripts/locatesdl2.cmake")

# Add cmake script dir to cmake's ""Path""
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(foray_example)

# Add subdirectories
add_subdirectory("foray")
add_subdirectory("examples")
