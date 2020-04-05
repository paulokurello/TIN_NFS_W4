cmake_minimum_required(VERSION 3.10)

project(NFS_W4)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(client_lib SHARED mynfs.cpp)
add_executable(server server.cpp)
