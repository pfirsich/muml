cmake_minimum_required(VERSION 3.12.4...4.2.3)

project(muml)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/wall.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/no-rtti.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/no-exceptions.cmake)

if(UTXT_ENABLE_ASAN)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/asan.cmake)
endif()

add_library(muml src/muml.cpp)
target_include_directories(muml PUBLIC include/)
muml_set_wall(muml)
muml_set_no_exceptions(muml)
muml_set_no_rtti(muml)

option(MUML_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
if(MUML_BUILD_TESTS)
  add_executable(muml-dump examples/dump.cpp examples/common.cpp)
  target_link_libraries(muml-dump PRIVATE muml)

  add_executable(parse-entities examples/parse-entities.cpp examples/common.cpp)
  target_link_libraries(parse-entities PRIVATE muml)

  add_executable(parse-entities-cpp examples/parse-entities-cpp.cpp examples/common.cpp)
  target_link_libraries(parse-entities-cpp PRIVATE muml)
endif()
