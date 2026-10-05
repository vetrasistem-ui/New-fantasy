cmake_minimum_required(VERSION 3.24)

project(FantasyClient VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

add_executable(fantasy-client
    Core/main.cpp
)
target_link_libraries(fantasy-client PRIVATE fantasy-client-core)

foreach(target fantasy-fmap fantasy-protocol fantasy-mapchunk-codec fantasy-network fantasy-client-core fantasy-client)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

enable_testing()
add_test(NAME fantasy-client-cli-smoke COMMAND fantasy-client)
