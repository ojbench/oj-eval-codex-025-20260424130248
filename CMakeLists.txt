cmake_minimum_required(VERSION 3.10)
project(oj_problem_025 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(code src/main.cpp vendor/lang.cpp)
target_include_directories(code PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
target_compile_options(code PRIVATE -O2 -Wall -Wextra -Wpedantic)
