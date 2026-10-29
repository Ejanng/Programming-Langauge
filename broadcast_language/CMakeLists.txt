cmake_minimum_required(VERSION 3.16)
project(broadcast_language C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(broadcast_language
  src/main.c
  src/arena.c
  src/errors.c
  src/lexer.c
  src/parser.c
  src/value.c
  src/interpreter.c)
if(MSVC)
  # MSVC: /W3 warnings; silence its "use fopen_s instead of fopen" deprecation warnings.
  target_compile_options(broadcast_language PRIVATE /W3)
  target_compile_definitions(broadcast_language PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  # GCC and Clang: the math functions (fmod) live in libm.
  target_link_libraries(broadcast_language PRIVATE m)
  target_compile_options(broadcast_language PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME examples COMMAND bash ${CMAKE_SOURCE_DIR}/tests/run_tests.sh $<TARGET_FILE:broadcast_language>)
