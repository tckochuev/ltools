cmake_minimum_required(VERSION 3.20)
project(ltools VERSION 1.0 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD 17)

enable_language(OBJCXX)

add_subdirectory(src)
