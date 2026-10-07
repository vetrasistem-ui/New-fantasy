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
set(PUGIXML_INSTALL OFF CACHE BOOL "" FORCE)
set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)

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

FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.16
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(sdl3 imgui pugixml)

add_library(fantasy-studio-project
    Project/ProjectManager.cpp
)

add_library(fantasy-map-core
    ../Shared/Formats/FMAP/FmapCore.cpp
    MapEngine/EditorOperations.cpp
    MapEngine/Core/MapStorage.cpp
    MapEngine/Core/SelectionModel.cpp
    MapEngine/Core/ActionHistory.cpp
    MapEngine/Core/MapDocument.cpp
    MapEngine/Commands/CommandValidator.cpp
    MapEngine/Commands/CommandExecutor.cpp
    MapEngine/Clipboard/MapClipboard.cpp
    MapEngine/Brushes/BrushEngine.cpp
    MapEngine/Query/MapQueryService.cpp
)

# F05.5 neutral asset contract plus isolated legacy binary readers. The readers
# feed the shared registry; no DAT/SPR/OTB type leaks into the canonical map core.
add_library(fantasy-asset-registry
    ../Shared/Assets/LegacyAssetRegistry.cpp
    ../Shared/Assets/LegacyAssetRegistryBuilder.cpp
    ../Shared/Assets/Legacy/DatReader.cpp
    ../Shared/Assets/Legacy/SprReader.cpp
    ../Shared/Assets/Legacy/OtbReader.cpp
)

# OTBM and its external XML files stay isolated from the canonical map core.
# They produce a neutral LegacyMapImportModel. The adapter below is the explicit
# compatibility boundary into the editor domain.
add_library(fantasy-legacy-import
    ../Shared/Formats/Legacy/OtbmReader.cpp
    ../Shared/Formats/Legacy/LegacyAuxXmlReader.cpp
)
target_link_libraries(fantasy-legacy-import PUBLIC pugixml::pugixml)

add_library(fantasy-map-legacy-bridge
    MapEngine/Import/LegacyMapAdapter.cpp
    MapEngine/Import/LegacyMapProjectLoader.cpp
)
target_link_libraries(fantasy-map-legacy-bridge PUBLIC
    fantasy-map-core
    fantasy-asset-registry
    fantasy-legacy-import
)

foreach(target fantasy-studio-project fantasy-map-core fantasy-asset-registry fantasy-legacy-import fantasy-map-legacy-bridge)
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
    fantasy-map-legacy-bridge
    fantasy-imgui
    SDL3::SDL3
)
# Give users a product-facing executable instead of an internal target name.
# The CMake target stays stable so existing build/test wiring does not change.
set_target_properties(fantasy-studio-gui PROPERTIES OUTPUT_NAME "FantasyStudio")
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

add_executable(fantasy-legacy-inspect
    ../Tools/LegacyInspect/main.cpp
)
target_include_directories(fantasy-legacy-inspect PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(fantasy-legacy-inspect PRIVATE fantasy-asset-registry fantasy-legacy-import)
if(MSVC)
    target_compile_options(fantasy-legacy-inspect PRIVATE /W4 /permissive-)
else()
    target_compile_options(fantasy-legacy-inspect PRIVATE -Wall -Wextra -Wpedantic)
endif()

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

add_executable(fantasy-map-core-v1-tests
    MapEngine/Core/MapCoreV1Tests.cpp
)
target_link_libraries(fantasy-map-core-v1-tests PRIVATE fantasy-map-core)

add_executable(fantasy-map-clipboard-tests
    MapEngine/Clipboard/MapClipboardTests.cpp
)
target_link_libraries(fantasy-map-clipboard-tests PRIVATE fantasy-map-core)

add_executable(fantasy-brush-engine-tests
    MapEngine/Brushes/BrushEngineTests.cpp
)
target_link_libraries(fantasy-brush-engine-tests PRIVATE fantasy-map-core)

add_executable(fantasy-map-legacy-adapter-tests
    MapEngine/Import/LegacyMapAdapterTests.cpp
)
target_link_libraries(fantasy-map-legacy-adapter-tests PRIVATE fantasy-map-legacy-bridge)

add_executable(fantasy-asset-registry-tests
    ../Shared/Assets/LegacyAssetRegistryTests.cpp
)
target_include_directories(fantasy-asset-registry-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(fantasy-asset-registry-tests PRIVATE fantasy-asset-registry)

add_executable(fantasy-legacy-binary-readers-tests
    ../Shared/Assets/Legacy/LegacyBinaryReadersTests.cpp
)
target_include_directories(fantasy-legacy-binary-readers-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(fantasy-legacy-binary-readers-tests PRIVATE fantasy-asset-registry)

add_executable(fantasy-otbm-reader-tests
    ../Shared/Formats/Legacy/OtbmReaderTests.cpp
)
target_include_directories(fantasy-otbm-reader-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(fantasy-otbm-reader-tests PRIVATE fantasy-legacy-import)

add_executable(fantasy-legacy-aux-xml-tests
    ../Shared/Formats/Legacy/LegacyAuxXmlReaderTests.cpp
)
target_include_directories(fantasy-legacy-aux-xml-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(fantasy-legacy-aux-xml-tests PRIVATE fantasy-legacy-import)

foreach(target fantasy-asset-registry-tests fantasy-legacy-binary-readers-tests fantasy-otbm-reader-tests fantasy-legacy-aux-xml-tests fantasy-map-core-v1-tests fantasy-map-clipboard-tests fantasy-brush-engine-tests fantasy-map-legacy-adapter-tests)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

enable_testing()
add_test(NAME fantasy-studio-project-tests COMMAND fantasy-studio-project-tests)
add_test(NAME fantasy-map-core-tests COMMAND fantasy-map-core-tests)
add_test(NAME fantasy-editor-operations-tests COMMAND fantasy-editor-operations-tests)
add_test(NAME fantasy-map-core-v1-tests COMMAND fantasy-map-core-v1-tests)
add_test(NAME fantasy-map-clipboard-tests COMMAND fantasy-map-clipboard-tests)
add_test(NAME fantasy-brush-engine-tests COMMAND fantasy-brush-engine-tests)
add_test(NAME fantasy-map-legacy-adapter-tests COMMAND fantasy-map-legacy-adapter-tests)
add_test(NAME fantasy-asset-registry-tests COMMAND fantasy-asset-registry-tests)
add_test(NAME fantasy-legacy-binary-readers-tests COMMAND fantasy-legacy-binary-readers-tests)
add_test(NAME fantasy-otbm-reader-tests COMMAND fantasy-otbm-reader-tests)
add_test(NAME fantasy-legacy-aux-xml-tests COMMAND fantasy-legacy-aux-xml-tests)
add_test(
    NAME fantasy-studio-open-project
    COMMAND fantasy-studio --open-project "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
add_test(
    NAME fantasy-studio-open-main-map
    COMMAND fantasy-studio --open-main-map "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
