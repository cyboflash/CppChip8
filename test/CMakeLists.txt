cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.10.0
    )
FetchContent_MakeAvailable(googletest)

# FetchContent_Declare(
#     chip8-test-rom
#     GIT_REPOSITORY https://github.com/corax89/chip8-test-rom.git
#     )
# FetchContent_MakeAvailable(chip8-test-rom)

# https://cliutils.gitlab.io/modern-cmake/chapters/testing/googletest.html
macro(package_add_test TESTNAME)
    # create an exectuable in which the tests will be stored
    add_executable(${TESTNAME} ${ARGN})
    set_property(TARGET ${TESTNAME} PROPERTY CXX_STANDARD 20)
    target_include_directories(${TESTNAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    # link the Google test infrastructure, mocking library, and a default main fuction to
    # the test executable.  Remove g_test_main if writing your own main function.
    target_link_libraries(${TESTNAME} gtest gmock gtest_main chip-8)
    # gtest_discover_tests replaces gtest_add_tests,
    # see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
    gtest_discover_tests(${TESTNAME}
        # set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${PROJECT_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
        )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER test)
endmacro()

package_add_test(test-chip8 test-chip8.cxx)
