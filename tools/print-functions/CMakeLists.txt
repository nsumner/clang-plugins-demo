
add_executable(print-functions
  main.cpp
)

if (LLVM_ENABLE_ASSERTIONS)
  add_definitions(-DLLVM_ENABLE_ASSERTIONS=${LLVM_ENABLE_ASSERTIONS})
endif()

llvm_map_components_to_libnames(REQ_LLVM_LIBRARIES ${LLVM_TARGETS_TO_BUILD}
                                core support option
)

target_link_libraries(print-functions
  function-printer
  clangAST
  clangBasic
  clangCodeGen
  clangLex
  clangParse
  clangSema
  clangAnalysis
  clangDriver
  clangFrontend
  clangAST
  clangEdit
  clangSerialization
  clangTooling
  clangToolingCore
  clangAST
  clangParse
  clangBasic
  clangSema
  clangAST
  ${REQ_LLVM_LIBRARIES}
)

target_include_directories(print-functions
  PRIVATE $<TARGET_PROPERTY:function-printer,INTERFACE_INCLUDE_DIRECTORIES>
)

# Platform dependencies.
if( WIN32 )
  find_library(SHLWAPI_LIBRARY shlwapi)
  target_link_libraries(print-functions
    ${SHLWAPI_LIBRARY}
  )
else()
  find_package(Threads REQUIRED)
  find_package(Curses REQUIRED)
  target_link_libraries(print-functions
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${CURSES_LIBRARIES}
  )
endif()

set_target_properties(print-functions
                      PROPERTIES
                      LINKER_LANGUAGE CXX
                      PREFIX ""
)

install(TARGETS print-functions
  RUNTIME DESTINATION bin
)

