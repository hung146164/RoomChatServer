cmake_minimum_required(VERSION 3.16)
project(ServerTCP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(header)

add_executable(server ./src/server.cpp)
add_executable(client ./src/client.cpp)
add_executable(client2 ./src/client2.cpp)
