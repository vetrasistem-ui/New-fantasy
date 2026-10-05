cmake_minimum_required(VERSION 3.24)

project(FantasyStudio VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ADR-012: pinned visual/platform dependencies for the native Fantasy editor.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    sdl3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG 829a65d769d935c4852f8159e964312c0957260a
    GIT_SHALLOW FALSE
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG f1cc2ae15e53a861a874c3034aae6798fde194ab
    GIT_SHALLOW FALSE
)

FetchContent_MakeAvailable(sdl3 imgui)

add_library(fantasy-studio-project
    Project/ProjectManager.cpp
)

add_library(fantasy-map-core
    MapEngine/FmapCore.cpp
)

foreach(target fantasy-studio-project fantasy-map-core)
    target_include_directories(${target} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

add_library(fantasy-imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp
)

target_include_directories(fantasy-imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(fantasy-imgui PUBLIC SDL3::SDL3)

if(MSVC)
    target_compile_options(fantasy-imgui PRIVATE /W3)
else()
    target_compile_options(fantasy-imgui PRIVATE -Wall -Wextra)
endif()

add_executable(fantasy-studio
    Core/main.cpp
)
target_link_libraries(fantasy-studio PRIVATE fantasy-studio-project fantasy-map-core)

add_executable(fantasy-studio-gui
    UI/EditorApp.cpp
)
target_link_libraries(fantasy-studio-gui PRIVATE
    fantasy-studio-project
    fantasy-map-core
    fantasy-imgui
    SDL3::SDL3
)

add_executable(fantasy-studio-project-tests
    Project/ProjectManagerTests.cpp
)
target_link_libraries(fantasy-studio-project-tests PRIVATE fantasy-studio-project)
target_compile_definitions(fantasy-studio-project-tests PRIVATE
    FANTASY_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_executable(fantasy-map-core-tests
    MapEngine/FmapCoreTests.cpp
)
target_link_libraries(fantasy-map-core-tests PRIVATE fantasy-map-core)
target_compile_definitions(fantasy-map-core-tests PRIVATE
    FANTASY_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)

enable_testing()
add_test(NAME fantasy-studio-project-tests COMMAND fantasy-studio-project-tests)
add_test(NAME fantasy-map-core-tests COMMAND fantasy-map-core-tests)
add_test(
    NAME fantasy-studio-open-project
    COMMAND fantasy-studio --open-project "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
add_test(
    NAME fantasy-studio-open-main-map
    COMMAND fantasy-studio --open-main-map "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
