
file(GENERATE
     OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/config.h"
     CONTENT
"
#ifndef RUNPLUGIN_CONFIG_H
#define RUNPLUGIN_CONFIG_H

#define PLUGIN_LIBRARY \"$<TARGET_FILE_NAME:streamchecker>\"
#define CMAKE_TEMP_LIBRARY_PATH \"${CMAKE_TEMP_LIBRARY_PATH}\"
#define LLVM_LIBRARY_DIRS \"${LLVM_LIBRARY_DIRS}\"

#endif
"
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

# NOTE: The order here was determined by configuring a build of clang with CMake
# and extracting the ordering from `libclang/CMakeFiles/libclang.dir/link.txt`
target_link_libraries(runstreamchecker
  PRIVATE
    clangAST
    clangBasic
    clangDriver
    clangFrontend
    clangIndex
    clangLex
    clangSema
    clangSerialization
    clangTooling

    clangFormat
    clangToolingInclusions
    clangToolingCore
    clangFrontend
    clangDriver

    clangRewriteFrontend
    clangStaticAnalyzerFrontend
    clangCrossTU
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerCore
    clangAST

    clangParse
    clangSerialization
    clangSema
    clangEdit
    clangRewrite
    clangAnalysis
    clangASTMatchers
    clangAST
    clangLex
    clangBasic

    ${REQ_LLVM_LIBRARIES}
)

target_include_directories(runstreamchecker
  PRIVATE
    $<TARGET_PROPERTY:streamchecker,INTERFACE_INCLUDE_DIRECTORIES>
)

# Platform dependencies.
if( WIN32 )
  find_library(SHLWAPI_LIBRARY shlwapi)
  target_link_libraries(runstreamchecker
    PRIVATE
      ${SHLWAPI_LIBRARY}
  )
else()
  find_package(Threads REQUIRED)
  find_package(Curses REQUIRED)
  target_link_libraries(runstreamchecker
    PRIVATE
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

