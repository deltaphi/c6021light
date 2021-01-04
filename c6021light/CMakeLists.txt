cmake_minimum_required (VERSION 3.6)
project (c6021light)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT MSVC)
  add_compile_options(-Wall -Wextra -pedantic)
endif()

option(ENABLE_COVERAGE "enable_language measurement option add_compile_definitions coverage" OFF)

if (ENABLE_COVERAGE)
    include(CodeCoverage.cmake)
    append_coverage_compiler_flags()
endif()

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include")
endif()

enable_testing()

# Now simply link against gtest or gtest_main as needed. Eg
include_directories("test")
include_directories(".")

add_executable(c6021lightTest
    "test/EmptyTest.cpp"
)
target_link_libraries(c6021lightTest gtest_main gmock)
add_test(NAME gtest_c6021lightTest_test COMMAND c6021lightTest)
target_compile_features(c6021lightTest PRIVATE cxx_std_14)

if (ENABLE_COVERAGE)
setup_target_for_coverage_gcovr_html(
  NAME c6021lightTest-gcovr
  EXECUTABLE ctest
  DEPENDENCIES c6021lightTest
  EXCLUDE build test
)
endif()