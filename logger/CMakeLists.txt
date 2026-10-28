cmake_minimum_required(VERSION 3.16)

project(imu_logger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(imu_core
  src/parser.cpp
)
target_include_directories(imu_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_options(imu_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(imu_logger
  src/main.cpp
  src/serial_port.cpp
)
target_link_libraries(imu_logger PRIVATE imu_core)
target_compile_options(imu_logger PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)
if(BUILD_TESTING)
  add_executable(imu_parser_tests
    tests/test_parser.cpp
  )
  target_link_libraries(imu_parser_tests PRIVATE imu_core)
  target_compile_options(imu_parser_tests PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME imu_parser_tests COMMAND imu_parser_tests)

  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(
      NAME imu_logger_e2e
      COMMAND ${Python3_EXECUTABLE}
              ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_serial_e2e.py
              $<TARGET_FILE:imu_logger>
    )
  endif()
endif()
