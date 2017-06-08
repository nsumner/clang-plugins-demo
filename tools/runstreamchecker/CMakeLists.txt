configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake" 
               "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(runstreamchecker
  main.cpp
)

if (LLVM_ENABLE_ASSERTIONS)
  add_definitions(-DLLVM_ENABLE_ASSERTIONS=${LLVM_ENABLE_ASSERTIONS})
endif()

llvm_map_components_to_libnames(REQ_LLVM_LIBRARIES ${LLVM_TARGETS_TO_BUILD}
                                core support option
)

target_link_libraries(runstreamchecker
  clangAST
  clangBasic
  clangLex
  clangParse
  clangAnalysis
  clangDriver
  clangFrontend
  clangASTMatchers
  clangRewriteFrontend
  clangRewrite
  clangStaticAnalyzerFrontend
  clangStaticAnalyzerCheckers
  clangStaticAnalyzerCore
  clangEdit
  clangSerialization
  clangTooling
  clangParse
  clangSema
  clangAST
  ${REQ_LLVM_LIBRARIES}
)

target_include_directories(runstreamchecker
  PRIVATE $<TARGET_PROPERTY:streamchecker,INTERFACE_INCLUDE_DIRECTORIES>
)

# Platform dependencies.
if( WIN32 )
  find_library(SHLWAPI_LIBRARY shlwapi)
  target_link_libraries(runstreamchecker
    ${SHLWAPI_LIBRARY}
  )
else()
  find_package(Threads REQUIRED)
  find_package(Curses REQUIRED)
  target_link_libraries(runstreamchecker
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${CURSES_LIBRARIES}
  )
endif()

set_target_properties(runstreamchecker
                      PROPERTIES
                      LINKER_LANGUAGE CXX
                      PREFIX ""
)

install(TARGETS runstreamchecker
  RUNTIME DESTINATION bin
)

