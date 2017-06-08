
project(clang-plugins-demo-tests)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(testlib
  files.c
  functions.c
)

