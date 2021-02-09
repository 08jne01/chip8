# CMakeList.txt : CMake project for Chip8_C, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
# Add source to this project's executable.
add_executable (c8 "Main.c"  "Chip8.c" "Chip8.h" "${DEPS}/SDL_FontCache/SDL_FontCache.c"  )

set(COPY_COMMAND "cp -r")

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
	target_sources(c8 PRIVATE "c8.rc")

	#Nobody got time for _s functions.
	add_definitions(-DIS_WINDOWS -D_CRT_SECURE_NO_WARNINGS)
	set(COPY_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cp.bat")

	#build without console
	target_link_options(c8 PRIVATE "/SUBSYSTEM:WINDOWS")
	#Sign exe after build on windows.
	set(SIGNSTR "Open Source Developer, Joshua Nelson")
	add_custom_command(TARGET c8 POST_BUILD COMMAND signtool sign /n ${SIGNSTR} /fd SHA256 /t http://timestamp.comodoca.com/authenticode  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/c8.exe)
endif()
target_link_libraries(c8 ${SDL2_LIB_ONLY} ${SDL2_TTF_LIBRARIES})



#This copies any files in the deps_build folder into the build location, this is to easily copy the dlls and font files.
#For building on linux you likely only need the font file provided you shared object is properly installed.
add_custom_command(TARGET c8 POST_BUILD COMMAND ${COPY_COMMAND} ${PROJECT_SOURCE_DIR}/deps_build  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

