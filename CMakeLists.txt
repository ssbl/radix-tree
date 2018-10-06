cmake_minimum_required(VERSION 3.4.3)
project(radix-tree VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(rt-tests radix_tree.cpp tests.cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(rt-tests PRIVATE "-g")
  target_compile_options(rt-tests PRIVATE "-O2")
  target_compile_options(rt-tests PRIVATE "-Werror")
  target_compile_options(rt-tests PRIVATE "-Weverything")
  target_compile_options(rt-tests PRIVATE "-Wno-c++98-compat")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(rt-tests PRIVATE "-g")
  target_compile_options(rt-tests PRIVATE "-Wall")
  target_compile_options(rt-tests PRIVATE "-Wextra")
endif()
