cmake_minimum_required(VERSION 3.24)

project(FantasyServer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fantasy-server-world
    Core/Scheduler.cpp
    Core/WorldRuntime.cpp
    ../Shared/Formats/FMAP/FmapCore.cpp
)

target_include_directories(fantasy-server-world PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(fantasy-server
    Core/main.cpp
)
target_link_libraries(fantasy-server PRIVATE fantasy-server-world)

target_compile_definitions(fantasy-server PRIVATE
    FANTASY_SERVER_VERSION="${PROJECT_VERSION}"
    FANTASY_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_executable(fantasy-server-world-tests
    Core/WorldRuntimeTests.cpp
)
target_link_libraries(fantasy-server-world-tests PRIVATE fantasy-server-world)
target_compile_definitions(fantasy-server-world-tests PRIVATE
    FANTASY_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)

foreach(target fantasy-server-world fantasy-server fantasy-server-world-tests)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

enable_testing()
add_test(NAME fantasy-server-smoke COMMAND fantasy-server --smoke-test)
add_test(NAME fantasy-server-world-tests COMMAND fantasy-server-world-tests)
