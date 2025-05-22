cmake_minimum_required(VERSION 3.31)
project(mini_x_messenger)
include_directories(include)

set(CMAKE_CXX_STANDARD 20)

add_executable(server src/server.cpp)
target_link_libraries(server ws2_32)
add_executable(client_sender src/client_sender.cpp)
target_link_libraries(client_sender ws2_32)
add_executable(client_receiver src/client_receiver.cpp)
target_link_libraries(client_receiver ws2_32)

