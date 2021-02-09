# CMakeList.txt : CMake project for Chip8_C, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (c8 "Main.c"  "Chip8.c" "Chip8.h" "${PROJECT_SOURCE_DIR}/deps/SDL_FontCache/SDL_FontCache.c" )
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
	set_property(TARGET c8 APPEND PROPERTY COMPILE_OPTIONS /NODEFAULTLIB)
endif()

target_link_libraries(c8 ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})

set(SIGNSTR "Open Source Developer, Joshua Nelson")

add_custom_command(TARGET c8 POST_BUILD
	COMMAND signtool sign /n ${SIGNSTR} /fd SHA256 /t http://timestamp.comodoca.com/authenticode  ${CMAKE_CURRENT_BINARY_DIR}/c8.exe)


