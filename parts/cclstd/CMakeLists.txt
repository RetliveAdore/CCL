cmake_minimum_required(VERSION 3.18)

project("CCL_Std" C CXX)

include_directories("./include")

file(GLOB_RECURSE CCL_Std_Files "./src/*.c" "./src/*.cpp" "./src/*.h" "./include/*.h")

link_libraries("CCL")

add_library("CCL_std" SHARED ${CCL_Std_Files})

if(MSVC)
  set_property(TARGET "CCL_std" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_dependencies(CCL_std CCL)