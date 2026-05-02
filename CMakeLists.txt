cmake_minimum_required(VERSION 3.10)
project(RoomChatApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#Header
include_directories(include)

file(GLOB SOURCES src/*.cpp)

add_executable(BackendServer ${SOURCES})
