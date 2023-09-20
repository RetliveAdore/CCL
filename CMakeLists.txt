cmake_minimum_required(VERSION 3.18)

project("CCL" C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin$<$<CONFIG:Debug>:_debug>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin$<$<CONFIG:Debug>:_debug>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin$<$<CONFIG:Debug>:_debug>")

add_compile_options(-fPIC)
add_link_options(-no-pie)

include_directories("./parts/cclcore/include")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  link_libraries(dl pthread X11 GL GLU)
endif()

add_subdirectory("./parts/cclcore")
add_subdirectory("./parts/cclstd")
add_subdirectory("./parts/cclcompressor")
add_subdirectory("./parts/cclgraphic")
add_subdirectory("./parts/demos")