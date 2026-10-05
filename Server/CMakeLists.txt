cmake_minimum_required(VERSION 3.24)

project(FantasyServer VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(fantasy-server
    Core/main.cpp
)

target_compile_definitions(fantasy-server PRIVATE
    FANTASY_SERVER_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
    target_compile_options(fantasy-server PRIVATE /W4 /permissive-)
else()
    target_compile_options(fantasy-server PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME fantasy-server-smoke COMMAND fantasy-server --smoke-test)
