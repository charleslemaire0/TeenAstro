cmake_minimum_required(VERSION 3.18)
project(teenastro_native CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(tetra SHARED tetra/tetra.cpp)

if(MSVC)
  target_compile_options(tetra PRIVATE /O2 /EHsc)
else()
  target_compile_options(tetra PRIVATE -O3 -fvisibility=hidden)
endif()
