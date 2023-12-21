cmake_minimum_required(VERSION 3.26)
project(bitpack)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
)

FetchContent_MakeAvailable(googletest)


add_library(bitpack INTERFACE)
target_compile_features(bitpack INTERFACE cxx_std_20)
target_include_directories(bitpack INTERFACE include/)

add_executable(bitpack_test test/main.cpp)
target_compile_options(bitpack_test PRIVATE -Wall -Wpedantic -Wextra -Werror)
target_link_libraries(bitpack_test gtest_main bitpack)
