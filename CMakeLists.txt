cmake_minimum_required(VERSION 2.6)
project(aqmodtools)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif()

include_directories(dep)
add_subdirectory(dep)
add_subdirectory(src)

