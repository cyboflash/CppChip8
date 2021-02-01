cmake_minimum_required(VERSION 3.16.3)

get_filename_component(Project ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "-" Project ${Project})
project(${Project} CXX)
message(STATUS "Project: " ${Project})

include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        7.1.3 
    )
FetchContent_MakeAvailable(fmt)

# FetchContent_Declare(
#     spdlog
#     GIT_REPOSITORY https://github.com/gabime/spdlog.git
#     GIT_TAG        v1.8.2
#     )
# FetchContent_MakeAvailable(spdlog)

set(SourceDir ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LibrarySources ${SourceDir}/Chip8.cxx)
set(ExecutableSources ${SourceDir}/main.cxx)
set(CompilationFlags -Wall -Werror -Wextra -Wpedantic -Wconversion -fmax-errors=3)
set(LinkLibraries fmt::fmt)
set(CxxStandard 20)

set(Executable ${Project}-main)
set(Library ${Project})

add_library(${Library} ${LibrarySources})
set_property(TARGET ${Library} PROPERTY CXX_STANDARD ${CxxStandard})
target_compile_options(${Library} PUBLIC ${CompilationFlags})
target_link_libraries(${Library} PUBLIC ${LinkLibraries})

add_executable(${Executable} ${ExecutableSources})
set_property(TARGET ${Executable} PROPERTY CXX_STANDARD ${CxxStandard})
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
