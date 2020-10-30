

cmake_minimum_required(VERSION 3.16)
project(bitpack)

add_library(bitpack INTERFACE)
target_compile_features(bitpack INTERFACE cxx_std_14)
target_include_directories(bitpack INTERFACE include/)

add_executable(bitpack_test test/main.cpp)
target_compile_options(bitpack_test PRIVATE -Wall -Wpedantic -Wextra -Werror)
target_link_libraries(bitpack_test bitpack)