cmake_minimum_required(VERSION 3.14)
project(DeadlockDetection)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(DeadlockDetection
        lock.cpp
        lock.hpp
        lock_manager.cpp
        lock_manager.hpp
        main.cpp)
