cmake_minimum_required(VERSION 3.20)
set(LANGS C CXX)
if(APPLE)
	list(APPEND LANGS OBJC OBJCXX)
endif()
project(ltools VERSION 1.0 LANGUAGES ${LANGS})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD 17)

if(APPLE)
	enable_language(OBJCXX)
endif()

add_subdirectory(src)
