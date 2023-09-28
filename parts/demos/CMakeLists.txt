cmake_minimum_required(VERSION 3.18)

project("Demos" C CXX)

include_directories("./include")
include_directories("../cclcore/include")
include_directories("../cclstd/include")
include_directories("../cclgraphic/include")
include_directories("../cclinet/include")
include_directories("../cclcompressor/include")

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

file(GLOB_RECURSE Demo_3
  "./src/demo3/*.c"
  "./src/demo3/*.cpp"
  "./include/demo3/*.h"
  "./include/demo3/*.hpp"
)

link_libraries("CCL")

add_executable("demo1" ${Demo_1})
add_executable("demo2" ${Demo_2})
add_executable("demo3" ${Demo_3})
add_executable("demo4_server" "./src/demo4/server.cpp")
add_executable("demo4_client" "./src/demo4/client.cpp")

if(MSVC)
  set_property(TARGET "demo1" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set_property(TARGET "demo2" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set_property(TARGET "demo3" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set_property(TARGET "demo4_server" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set_property(TARGET "demo4_client" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_dependencies(demo1 CCL_std)
add_dependencies(demo2 CCL_std CCL_graphic)
add_dependencies(demo3 CCL_Compressor)
add_dependencies(demo4_server CCL_std CCL_inet)
add_dependencies(demo4_client CCL_std CCL_inet)
