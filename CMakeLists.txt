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

file(GLOB Sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx)

set(Executable ${Project}-main)

add_executable(${Executable} ${Sources})
set_property(TARGET ${Executable} PROPERTY CXX_STANDARD 20)
target_compile_options(${Executable} PRIVATE -Wall -Werror -Wextra -Wpedantic -Wconversion -fmax-errors=3)
target_link_libraries(${Executable} PRIVATE fmt::fmt)

option(BUILD_TEST_PACKAGE "Build unit tests" ON)

if (BUILD_TEST_PACKAGE)
    list(FILTER Sources EXCLUDE REGEX "main\.cxx")

    set(Library ${Project})
    add_library(${Library} ${Sources})
    set_property(TARGET ${Library} PROPERTY CXX_STANDARD 20)
    target_compile_options(${Library} PUBLIC -Wall -Werror -Wextra -pedantic -fmax-errors=3)
    target_link_libraries(${Library} PUBLIC fmt::fmt)

    enable_testing()
    include(GoogleTest)
    add_subdirectory(test)

endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
