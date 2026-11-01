cmake_minimum_required(VERSION 3.20)
project(chess LANGUAGES CXX)

find_packages(SFML 3.1 REQUIRED COMPONENTS Graphics Window System)

add_executable(chess src/main.cpp)

