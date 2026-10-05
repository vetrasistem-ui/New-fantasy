cmake_minimum_required(VERSION 3.24)

project(FantasyStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fantasy-studio-project
    Project/ProjectManager.cpp
)

target_include_directories(fantasy-studio-project PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
    target_compile_options(fantasy-studio-project PRIVATE /W4 /permissive-)
else()
    target_compile_options(fantasy-studio-project PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(fantasy-studio
    Core/main.cpp
)

target_link_libraries(fantasy-studio PRIVATE fantasy-studio-project)

add_executable(fantasy-studio-project-tests
    Project/ProjectManagerTests.cpp
)

target_link_libraries(fantasy-studio-project-tests PRIVATE fantasy-studio-project)
target_compile_definitions(fantasy-studio-project-tests PRIVATE
    FANTASY_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)

enable_testing()
add_test(NAME fantasy-studio-project-tests COMMAND fantasy-studio-project-tests)
add_test(
    NAME fantasy-studio-open-project
    COMMAND fantasy-studio --open-project "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
add_test(
    NAME fantasy-studio-open-main-map
    COMMAND fantasy-studio --open-main-map "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
