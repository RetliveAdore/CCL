cmake_minimum_required(VERSION 3.18)

project("CCL_Core" C CXX)

include_directories("./include")

file(GLOB_RECURSE CCL_Core_Files "./src/*.c" "./src/*.cpp" "./src/*.h" "./include/*.h")

add_library("CCL" STATIC ${CCL_Core_Files})

if(MSVC)
  set_property(TARGET "CCL" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()