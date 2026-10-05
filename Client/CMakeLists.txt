cmake_minimum_required(VERSION 3.24)

project(FantasyClient VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Native 2D baseline: SDL3 for platform/input, SDL_Renderer3 for rendering,
# Dear ImGui for presentation. Gameplay remains in the shared client core.
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

add_library(fantasy-fmap
    ../Shared/Formats/FMAP/FmapCore.cpp
)
target_include_directories(fantasy-fmap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(fantasy-protocol
    ../Shared/Protocol/FantasyProtocol.cpp
)
target_include_directories(fantasy-protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(fantasy-mapchunk-codec
    ../Shared/Protocol/MapChunkPayload.cpp
)
target_include_directories(fantasy-mapchunk-codec PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(fantasy-mapchunk-codec PUBLIC fantasy-protocol fantasy-fmap)

add_library(fantasy-network
    ../Shared/Network/TcpTransport.cpp
    ../Shared/Network/FrameStream.cpp
)
target_include_directories(fantasy-network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(fantasy-network PUBLIC fantasy-protocol)
if(WIN32)
    target_link_libraries(fantasy-network PUBLIC ws2_32)
endif()

add_library(fantasy-client-core
    Core/DevelopmentClient.cpp
)
target_include_directories(fantasy-client-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(fantasy-client-core PUBLIC
    fantasy-fmap
    fantasy-protocol
    fantasy-mapchunk-codec
    fantasy-network
)

add_library(fantasy-client-imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(fantasy-client-imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(fantasy-client-imgui PUBLIC SDL3::SDL3)

add_executable(fantasy-client
    Core/main.cpp
)
target_link_libraries(fantasy-client PRIVATE fantasy-client-core)

add_executable(fantasy-client-gui
    UI/ClientApp.cpp
)
target_link_libraries(fantasy-client-gui PRIVATE
    fantasy-client-core
    fantasy-client-imgui
    SDL3::SDL3
)

foreach(target
    fantasy-fmap
    fantasy-protocol
    fantasy-mapchunk-codec
    fantasy-network
    fantasy-client-core
    fantasy-client-imgui
    fantasy-client
    fantasy-client-gui)
    if(MSVC)
        if(target STREQUAL "fantasy-client-imgui")
            target_compile_options(${target} PRIVATE /W3)
        else()
            target_compile_options(${target} PRIVATE /W4 /permissive-)
        endif()
    else()
        if(target STREQUAL "fantasy-client-imgui")
            target_compile_options(${target} PRIVATE -Wall -Wextra)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()
endforeach()

enable_testing()
add_test(NAME fantasy-client-cli-smoke COMMAND fantasy-client)
add_test(NAME fantasy-client-gui-cli-smoke COMMAND fantasy-client-gui)
