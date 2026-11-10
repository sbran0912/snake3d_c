cmake_minimum_required(VERSION 3.16)

# Fester Compiler: clang. (Der parallele Makefile-Build kompiliert mit gcc.)
# Achtung: greift nur beim ersten Konfigurieren - ein bereits mit gcc
# konfiguriertes build/ muss vorher geloescht werden.
set(CMAKE_C_COMPILER clang)

project(path3d C)

# Gleiche Sprachversion wie im Makefile: striktes C17 (-std=c17).
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Beim Konfigurieren compile_commands.json fuer clangd erzeugen.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(path3d
    lib3d/glcore.c
    lib3d/lib3d_util.c
    lib3d/lib3d.c
    app/snake.c
    app/main.c
)

target_include_directories(path3d PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib3d
)

target_compile_options(path3d PRIVATE -Wall -Wextra -O2)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(path3d PRIVATE glfw OpenGL::GL m)
