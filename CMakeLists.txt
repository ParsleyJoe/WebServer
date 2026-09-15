cmake_minimum_required(VERSION 3.25)

project(NetworkServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_executable(network_server)

target_sources(network_server
    PRIVATE
        src/main.cpp
        
        include/http.hpp
)

target_include_directories(network_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (MSVC)
    target_compile_options(network_server PRIVATE /W4)
else()
    target_compile_options(network_server PRIVATE -Wall -Wextra -Wpedantic)
endif()
