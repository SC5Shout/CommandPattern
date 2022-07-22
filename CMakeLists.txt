cmake_minimum_required(VERSION 3.22)
project(CommandPattern)

set(CMAKE_CXX_STANDARD 23)

add_executable(CommandPattern main.cpp CommandQueue.cpp TicketMutex.h RingBuffer.h)
