add_library(streamchecker MODULE
  SimpleStreamChecker.cpp
)

target_include_directories(streamchecker
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS streamchecker DESTINATION lib)

