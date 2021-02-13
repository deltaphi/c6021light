cmake_minimum_required (VERSION 3.6)
project (c6021light)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT MSVC)
  add_compile_options(-Wall -Wextra -pedantic)
endif()

if (WIN32)
    # Disable Windows libc depreciation warnings for POSIX functions used in the embedded code.
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if (WIN32 AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  # Let clang accept a __try {
  add_compile_options(-Wno-error=language-extension-token)
endif()

option(ENABLE_COVERAGE "enable_language measurement option add_compile_definitions coverage" OFF)
option(ENABLE_CLANG_TIDY "Enable static code analysis using clang-tidy" OFF)

if (ENABLE_COVERAGE)
    include(CodeCoverage.cmake)
    append_coverage_compiler_flags()
endif()

if (ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE clang-tidy)
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
set(RR32CAN_INCLUDE_DIR ".pio/libdeps/bluepill/RR32CanLibrary/lib")
set(RR32CAN_LIB_DIR "${RR32CAN_INCLUDE_DIR}/RR32Can")
file(GLOB_RECURSE RR32CAN_LIB_SRCS "${RR32CAN_LIB_DIR}/*.cpp")

set(RR32CANENGINEDB_LIB_DIR "${RR32CAN_INCLUDE_DIR}/RR32CanEngineDb")
file(GLOB_RECURSE RR32CANENGINEDB_LIB_SRCS "${RR32CANENGINEDB_LIB_DIR}/*.cpp")

set(ATOMIC_RING_BUFFER_INCLUDE_DIR ".pio/libdeps/bluepill/AtomicRingBuffer")


include_directories("test")
include_directories("test/mocks")
include_directories(".pio/libdeps/bluepill/LocoNet/utility")
include_directories("${RR32CAN_INCLUDE_DIR}")
include_directories("${ATOMIC_RING_BUFFER_INCLUDE_DIR}")
include_directories("lib/microrl/src")

include_directories("src")

add_executable(c6021lightTest
    "test/unit/EmptyTest.cpp"
    "test/unit/AccessoryPacketTest.cpp"
    "test/integration/StatelessRouting.cpp"
    "test/integration/StopGoRequest.cpp"
    "test/mocks/LocoNet.cpp"
    "test/mocks/RoutingForwarderMock.cpp"
    
    "src/tasks/RoutingTask/CANForwarder.cpp"
    "src/tasks/RoutingTask/I2CForwarder.cpp"
    "src/tasks/RoutingTask/LocoNetForwarder.cpp"
    "src/tasks/RoutingTask/LocoNetHelpers.cpp"
    "src/tasks/RoutingTask/LocoNetPrinter.cpp"
    "src/tasks/RoutingTask/LocoNetSlotServer.cpp"
    "src/tasks/RoutingTask/CanEngineDB.cpp"
    "src/tasks/RoutingTask/RoutingTask.cpp"

    "src/MarklinI2C/Messages/AccessoryMsg.cpp"

    "${RR32CAN_LIB_SRCS}"
    "${RR32CANENGINEDB_LIB_SRCS}"
)
target_link_libraries(c6021lightTest gtest_main gmock)

if (ENABLE_CLANG_TIDY AND CLANG_TIDY_EXECUTABLE)
    set_target_properties(c6021lightTest PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
endif()

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
