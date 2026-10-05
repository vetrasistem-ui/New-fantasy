cmake_minimum_required(VERSION 3.24)

project(FantasyServer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fantasy-protocol
    ../Shared/Protocol/FantasyProtocol.cpp
)
target_include_directories(fantasy-protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_library(fantasy-server-world
    Core/Scheduler.cpp
    Core/WorldRuntime.cpp
    ../Shared/Formats/FMAP/FmapCore.cpp
)
target_include_directories(fantasy-server-world PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_library(fantasy-server-session
    Network/DevelopmentSession.cpp
)
target_include_directories(fantasy-server-session PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(fantasy-server-session PUBLIC fantasy-server-world fantasy-protocol)

add_executable(fantasy-server
    Core/main.cpp
)
target_link_libraries(fantasy-server PRIVATE fantasy-server-world fantasy-protocol fantasy-server-session)

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

add_executable(fantasy-protocol-tests
    ../Shared/Protocol/FantasyProtocolTests.cpp
)
target_link_libraries(fantasy-protocol-tests PRIVATE fantasy-protocol)

add_executable(fantasy-development-session-tests
    Network/DevelopmentSessionTests.cpp
)
target_link_libraries(fantasy-development-session-tests PRIVATE fantasy-server-session)
target_compile_definitions(fantasy-development-session-tests PRIVATE
    FANTASY_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)

foreach(target
    fantasy-protocol
    fantasy-server-world
    fantasy-server-session
    fantasy-server
    fantasy-server-world-tests
    fantasy-protocol-tests
    fantasy-development-session-tests)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

enable_testing()
add_test(NAME fantasy-server-smoke COMMAND fantasy-server --smoke-test)
add_test(NAME fantasy-server-world-tests COMMAND fantasy-server-world-tests)
add_test(NAME fantasy-protocol-tests COMMAND fantasy-protocol-tests)
add_test(NAME fantasy-development-session-tests COMMAND fantasy-development-session-tests)
