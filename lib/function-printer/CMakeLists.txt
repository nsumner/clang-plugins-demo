add_library(function-printer-plugin MODULE
  PrintFunctionsAction.cpp
)
add_library(function-printer STATIC
  PrintFunctionsAction.cpp
)

target_include_directories(function-printer
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS function-printer DESTINATION lib)

