cmake_minimum_required(VERSION 3.20)
project(chess LANGUAGES CXX)

find_package(SFML 3.1 REQUIRED COMPONENTS Graphics Window System)

add_executable(chess src/main.cpp)
target_compile_features(chess PRIVATE cxx_std_20)
target_link_libraries(chess PRIVATE SFML::Graphics SFML::Window SFML::System)
target_compile_definitions(chess PRIVATE CHESS_ASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\")
set_target_properties(chess PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(chess PRIVATE /W4 /permissive-)
else()
    target_compile_options(chess PRIVATE -Wall -Wextra -Wpedantic)
endif()

