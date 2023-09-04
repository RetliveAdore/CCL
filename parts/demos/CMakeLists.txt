cmake_minimum_required(VERSION 3.18)

project("Demos" C CXX)

include_directories("./include")
include_directories("../cclcore/include")
include_directories("../cclstd/include")
include_directories("../cclgraphic/include")

file(GLOB_RECURSE Demo_1
  "./src/demo1/*.c"
  "./src/demo1/*.cpp"
  "./include/demo1/*.h"
  "./include/demo1/*.hpp"
)

file(GLOB_RECURSE Demo_2
  "./src/demo2/*.c"
  "./src/demo2/*.cpp"
  "./include/demo2/*.h"
  "./include/demo2/*.hpp"
)

link_libraries("CCL")

add_executable("demo1" ${Demo_1})
add_executable("demo2" ${Demo_2})

if(MSVC)
  set_property(TARGET "demo1" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set_property(TARGET "demo2" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_dependencies(demo1 CCL_std)
add_dependencies(demo2 CCL_std CCL_graphic)
