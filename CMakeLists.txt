cmake_minimum_required(VERSION 3.4.3)
project(radix-tree VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(radix-tree radix_tree.cpp radix_tree.hpp)

enable_testing()
add_subdirectory(test)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(rt-tests PRIVATE "-Weverything")
  target_compile_options(rt-tests PRIVATE "-Wno-c++98-compat")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(rt-tests PRIVATE "-Wall")
  target_compile_options(rt-tests PRIVATE "-Wextra")
endif()
