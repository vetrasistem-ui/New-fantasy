cmake_minimum_required(VERSION 3.24)
project(FantasyAssetCandidates LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(JSON_BuildTests OFF CACHE INTERNAL "" FORCE)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

set(FANTASY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")

add_executable(fantasy-asset-candidates
    main.cpp
    ${FANTASY_ROOT}/Shared/Assets/LegacyAssetRegistry.cpp
    ${FANTASY_ROOT}/Shared/Assets/LegacyAssetRegistryBuilder.cpp
    ${FANTASY_ROOT}/Shared/Assets/Legacy/DatReader.cpp
    ${FANTASY_ROOT}/Shared/Assets/Legacy/SprReader.cpp
    ${FANTASY_ROOT}/Shared/Assets/Legacy/OtbReader.cpp
    ${FANTASY_ROOT}/Shared/Formats/Legacy/OtbmReader.cpp
    ${FANTASY_ROOT}/Shared/Formats/Legacy/OtbmStreamReader.cpp
)

target_include_directories(fantasy-asset-candidates PRIVATE ${FANTASY_ROOT})
target_link_libraries(fantasy-asset-candidates PRIVATE nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(fantasy-asset-candidates PRIVATE /W4 /permissive-)
else()
    target_compile_options(fantasy-asset-candidates PRIVATE -Wall -Wextra -Wpedantic)
endif()
