cmake_minimum_required(VERSION 3.18)

project("CCL_Inet" C CXX)

include_directories("./include")
include_directories("../cclstd/include")

file(GLOB_RECURSE CCL_Inet_Files
"./src/*.c"
"./src/*.cpp"
"./src/*.h"
"./src/*.hpp"
"./include/*.h"
)

link_libraries("CCL")

add_library("CCL_inet" SHARED ${CCL_Inet_Files})

if(MSVC)
  set_property(TARGET "CCL_inet" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_dependencies(CCL_inet CCL)