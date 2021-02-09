# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project (Chip8_C)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} "deps")

set(DEPS "${CMAKE_CURRENT_LIST_DIR}/deps")
# Include sub-projects.
add_subdirectory ("src")
