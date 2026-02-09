cmake_minimum_required(VERSION 3.20)
project(kvstore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KV_ENABLE_SANITIZERS "Enable ASan/UBSan (clang recommended)" ON)

if(KV_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address,undefined)
endif()

include(FetchContent)

add_library(kvstore
  src/kvstore.cpp
)
target_include_directories(kvstore PUBLIC include)
target_compile_options(kvstore PRIVATE -Wall -Wextra -Wpedantic)

add_executable(kvserver src/main.cpp)
target_link_libraries(kvserver PRIVATE kvstore)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(kv_tests
  tests/kvstore_test.cpp
)
target_link_libraries(kv_tests PRIVATE kvstore GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(kv_tests)

add_executable(microbench tools/bench/microbench.cpp)
target_link_libraries(microbench PRIVATE kvstore)
