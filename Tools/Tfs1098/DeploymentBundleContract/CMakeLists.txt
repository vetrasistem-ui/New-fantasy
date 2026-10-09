cmake_minimum_required(VERSION 3.24)
project(FantasyTfs1098DeploymentBundleContract LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(fantasy-tfs1098-deployment-contract main.cpp)
target_include_directories(fantasy-tfs1098-deployment-contract PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

if(MSVC)
    target_compile_options(fantasy-tfs1098-deployment-contract PRIVATE /W4 /permissive-)
else()
    target_compile_options(fantasy-tfs1098-deployment-contract PRIVATE -Wall -Wextra -Wpedantic)
endif()
