cmake_minimum_required(VERSION 3.10)

project(NFS_W4)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-O1 -fPIE -ggdb -Wall -Wextra -Wno-unused-parameter -Werror -Wpedantic -pthread")

add_executable(server server.cpp)

add_library(client_lib SHARED mynfs.cpp)

add_executable(client_test client_test.cpp)
target_link_libraries(client_test client_lib)

