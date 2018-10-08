add_executable(rt-tests
  tests.cpp
  unit_tests.cpp
  fuzz_tests.cpp)
target_link_libraries(rt-tests radix-tree)
target_include_directories(rt-tests PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(rt-tests SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/external)

add_test(NAME AllTests COMMAND rt-tests)
