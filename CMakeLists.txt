cmake_minimum_required(VERSION 3.3)
project(chatUDP_winsrv)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/skel.h src/etcp.h src/sofuncs.c src/wincompat.c src/miscfuncs.c src/main.cpp src/iofuncs.c src/automutex.h src/ClientRec.cpp src/ClientRec.h)
add_executable(chatUDP_winsrv ${SOURCE_FILES})
target_link_libraries(chatUDP_winsrv ws2_32)