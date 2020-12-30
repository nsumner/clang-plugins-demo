add_library(streamchecker MODULE
  SimpleStreamChecker.cpp
)

set_target_properties(streamchecker PROPERTIES
  CXX_VISIBILITY_PRESET hidden
)

target_include_directories(streamchecker
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(streamchecker
  PRIVATE
    clang-cpp
)

install(TARGETS streamchecker DESTINATION lib)

