cmake_minimum_required(VERSION 3.24)

project(FantasyStudio VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Native 2D baseline: SDL3 for platform/input, SDL_Renderer3 for rendering,
# Dear ImGui for editor tooling. Exact dependency pins are in docs/UPSTREAMS.md.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    sdl3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.18
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9b
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(sdl3 imgui)

add_library(fantasy-studio-project
    Project/ProjectManager.cpp
)

add_library(fantasy-map-core
    ../Shared/Formats/FMAP/FmapCore.cpp
    MapEngine/EditorOperations.cpp
)

# F05.5 neutral asset contract. Binary DAT/SPR/OTB readers are intentionally
# kept out of this library and will feed it from the legacy compatibility layer.
add_library(fantasy-asset-registry
    ../Shared/Assets/LegacyAssetRegistry.cpp
)

foreach(target fantasy-studio-project fantasy-map-core fantasy-asset-registry)
    target_include_directories(${target} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
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
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
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

# V5 implements the approved visual shell. Earlier revisions remain in Git history.
add_executable(fantasy-studio-gui
    UI/StudioAppV5.cpp
    UI/StudioTheme.cpp
)
target_link_libraries(fantasy-studio-gui PRIVATE
    fantasy-studio-project
    fantasy-map-core
    fantasy-asset-registry
    fantasy-imgui
    SDL3::SDL3
)
if(WIN32)
    target_link_libraries(fantasy-studio-gui PRIVATE dwmapi)
endif()

# Presentation-only resources travel with the executable; no project-root or
# machine-specific absolute resource path is baked into the Studio.
add_custom_target(fantasy-studio-presentation
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/UI/Assets"
        "$<TARGET_FILE_DIR:fantasy-studio-gui>/StudioVisual"
)
add_dependencies(fantasy-studio-gui fantasy-studio-presentation)

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

add_executable(fantasy-editor-operations-tests
    MapEngine/EditorOperationsTests.cpp
)
target_link_libraries(fantasy-editor-operations-tests PRIVATE fantasy-map-core)
target_compile_definitions(fantasy-editor-operations-tests PRIVATE
    FANTASY_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_executable(fantasy-asset-registry-tests
    ../Shared/Assets/LegacyAssetRegistryTests.cpp
)
target_include_directories(fantasy-asset-registry-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(fantasy-asset-registry-tests PRIVATE fantasy-asset-registry)
if(MSVC)
    target_compile_options(fantasy-asset-registry-tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(fantasy-asset-registry-tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME fantasy-studio-project-tests COMMAND fantasy-studio-project-tests)
add_test(NAME fantasy-map-core-tests COMMAND fantasy-map-core-tests)
add_test(NAME fantasy-editor-operations-tests COMMAND fantasy-editor-operations-tests)
add_test(NAME fantasy-asset-registry-tests COMMAND fantasy-asset-registry-tests)
add_test(
    NAME fantasy-studio-open-project
    COMMAND fantasy-studio --open-project "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
add_test(
    NAME fantasy-studio-open-main-map
    COMMAND fantasy-studio --open-main-map "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
