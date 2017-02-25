cmake_minimum_required(VERSION 3.6)
project(starter_code)

set(CMAKE_CXX_STANDARD 11)

set(SERVER_SRC
        echo_server.c utility.cpp utility.h)

set(CLIENT_SRC
        echo_client.c utility.cpp utility.h)

add_executable(starter_server ${SERVER_SRC})
add_executable(starter_client ${CLIENT_SRC})
