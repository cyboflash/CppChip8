cmake_minimum_required(VERSION 3.19.5)

# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/sdl2-cmake-scripts)

get_filename_component(Project ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "-" Project ${Project})
project(${Project} CXX)
message(STATUS "Project: " ${Project})

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        7.1.3 
    )
FetchContent_MakeAvailable(fmt)
if (NOT fmt_POPULATED)
    FetchContent_Populate(fmt)
    add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})
endif()
message(STATUS "fmt_SOURCE_DIR = ${fmt_SOURCE_DIR}")
message(STATUS "fmt_BINARY_DIR = ${fmt_BINARY_DIR}")

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.8.2
    )
FetchContent_MakeAvailable(spdlog)
if (NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)
    set(SPDLOG_BUILD_EXAMPLES OFF)
    set(SPDLOG_BUILD_BENCH    OFF)
    set(SPDLOG_BUILD_TESTS    OFF)
    add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()
message(STATUS "spdlog_SOURCE_DIR = ${spdlog_SOURCE_DIR}")
message(STATUS "spdlog_BINARY_DIR = ${spdlog_BINARY_DIR}")

# FetchContent_Declare(
#     sdl
#     GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
#     GIT_TAG release-2.0.14
# )
# FetchContent_MakeAvailable(sdl)
# if (NOT sdl_POPULATED)
#     FetchContent_Populate(sdl)
#     add_subdirectory(${sdl_SOURCE_DIR} ${sdl_BINARY_DIR})
# endif()
# message(STATUS "sdl_SOURCE_DIR = ${sdl_SOURCE_DIR}")
# message(STATUS "sdl_BINARY_DIR = ${sdl_BINARY_DIR}")
# message(STATUS "SDL2_LIBRARIES = ${SDL2_LIBRARIES}")
# message(STATUS "SDL2_LIBRARY = ${SDL2_LIBRARY}")
# message(STATUS "SDL2_INCLUDE_DIRS = ${SDL2_INCLUDE_DIRS}")
# message(STATUS "SDL2_INCLUDE_DIR = ${SDL2_INCLUDE_DIR}")

# FetchContent_Declare(
#     sdl_mixer
#     GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
#     GIT_TAG release-2.0.4
# )
# FetchContent_MakeAvailable(sdl_mixer)
#
# FetchContent_Declare(
#     sdl_image
#     GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
#     GIT_TAG release-2.0.5
# )
# FetchContent_MakeAvailable(sdl_image)

find_package(SDL2 REQUIRED)

set(SourceDir ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LibrarySources 
    ${SourceDir}/Chip8.cxx
    ${SourceDir}/Chip8Emulator.cxx
    )

set(ExecutableSources ${SourceDir}/main.cxx)
set(CompilationFlags -Wall -Werror -Wextra -Wpedantic -Wconversion -Wundef -fmax-errors=3)
set(LinkLibraries fmt::fmt ${SDL2_LIBRARIES} spdlog::spdlog)

set(Executable ${Project}-main)
set(Library ${Project})

add_library(${Library} ${LibrarySources})
target_compile_options(${Library} PUBLIC ${CompilationFlags})
target_link_libraries(${Library} PUBLIC ${LinkLibraries})
target_include_directories(${Library} PUBLIC ${SDL2_INCLUDE_DIRS})

add_executable(${Executable} ${ExecutableSources})
target_compile_options(${Executable} PRIVATE ${CompilationFlags})
target_link_libraries(${Executable} PRIVATE ${Library} ${LinkLibraries})

option(BUILD_TEST_PACKAGE "Build unit tests" ON)

if (BUILD_TEST_PACKAGE)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(test)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
