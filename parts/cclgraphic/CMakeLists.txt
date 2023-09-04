cmake_minimum_required(VERSION 3.18)

project("CCL_Graphic" C CXX)

include_directories("./include")
include_directories("../cclstd/include")

file(GLOB_RECURSE CCL_Graphic_Files "./src/*.c" "./src/*.cpp" "./src/*.h" "./include/*.h")

link_libraries("CCL")

add_library("CCL_graphic" SHARED ${CCL_Graphic_Files})

if(MSVC)
  set_property(TARGET "CCL_graphic" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_dependencies(CCL_graphic CCL)
