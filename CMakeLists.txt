cmake_minimum_required(VERSION 3.10)

project(NFS_W4)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-O1 -ggdb -Wall -Wextra -Wno-unused-parameter -Werror -Wpedantic -pthread")

add_executable(server server.cpp)

add_library(client_lib SHARED mynfs.cpp)

add_executable(client_test client_test.cpp)
target_link_libraries(client_test client_lib)

add_executable(fail_test fail_test.cpp)
target_link_libraries(fail_test client_lib)

add_executable(upload upload.cpp)
target_link_libraries(upload client_lib)

add_executable(download download.cpp)
target_link_libraries(download client_lib)

add_executable(tree tree.cpp)
target_link_libraries(tree client_lib)

add_executable(shell shell.cpp)
target_link_libraries(shell client_lib)
