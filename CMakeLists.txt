cmake_minimum_required(VERSION 3.0)
project(sim CXX)

## C++14 Rocks!
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(src)
