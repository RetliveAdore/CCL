cmake_minimum_required(VERSION 3.18)

project("CCL_Compressor" C CXX)

include_directories("./include")
include_directories("../cclstd/include")

file(GLOB_RECURSE CCL_Coder_Files "./src/*.c" "./src/*.cpp" "./src/*.h" "./include/*.h")

link_libraries("CCL")

add_library("CCL_Compressor" SHARED ${CCL_Coder_Files})

if(MSVC)
  set_property(TARGET "CCL_Compressor" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_dependencies(CCL_Compressor CCL)
