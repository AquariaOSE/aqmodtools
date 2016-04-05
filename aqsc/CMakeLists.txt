cmake_minimum_required(VERSION 2.6)
PROJECT(aqsc)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

add_subdirectory(src)
