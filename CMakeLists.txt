cmake_minimum_required(VERSION 3.20)
project(Kutie VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
    set(KUTIE_DPI_MANIFEST "<?xml version='1.0' encoding='UTF-8' standalone='yes'?>
<assembly xmlns='urn:schemas-microsoft-com:asm.v1' manifestVersion='1.0'>
  <application xmlns='urn:schemas-microsoft-com:asm.v3'>
    <windowsSettings>
      <dpiAware xmlns='http://schemas.microsoft.com/SMI/2005/WindowsSettings'>true/pm</dpiAware>
      <dpiAwareness xmlns='http://schemas.microsoft.com/SMI/2016/WindowsSettings'>PerMonitorV2</dpiAwareness>
    </windowsSettings>
  </application>
  <compatibility xmlns='urn:schemas-microsoft-com:compatibility.v1'>
    <application>
      <supportedOS Id='{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}'/>
    </application>
  </compatibility>
</assembly>")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
    file(WRITE "${CMAKE_BINARY_DIR}/generated/dpi_awareness.manifest" "${KUTIE_DPI_MANIFEST}")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows")
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows")
endif()

find_package(nlohmann_json CONFIG REQUIRED)

set(WEBVIEW2_SEARCH_PATHS
    "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows"
    "$ENV{VCPKG_ROOT}/installed/x64-windows"
    "E:/vcpkg/installed/x64-windows"
    "C:/vcpkg/installed/x64-windows"
)

find_path(WEBVIEW2_INCLUDE_DIR NAMES WebView2.h PATH_SUFFIXES include PATHS ${WEBVIEW2_SEARCH_PATHS})
find_library(WEBVIEW2_LIBRARY NAMES WebView2LoaderStatic WebView2Loader.dll.lib PATH_SUFFIXES lib PATHS ${WEBVIEW2_SEARCH_PATHS})

if(NOT WEBVIEW2_INCLUDE_DIR)
    message(FATAL_ERROR "WebView2 SDK not found. Install with: vcpkg install webview2:x64-windows")
endif()

set(FRONTEND_DIR "${CMAKE_SOURCE_DIR}/sample/frontend")
set(GEN_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GEN_DIR}")

find_package(Python3 COMPONENTS Interpreter)
set(GENERATED_ASSET_SOURCES "")
if(Python3_Interpreter_FOUND)
    file(GLOB_RECURSE FRONTEND_FILES CONFIGURE_DEPENDS "${FRONTEND_DIR}/*")
    add_custom_command(
        OUTPUT "${GEN_DIR}/assets.rc" "${GEN_DIR}/asset_loader.cpp" "${GEN_DIR}/asset_ids.h"
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/embed_assets.py"
                "${FRONTEND_DIR}" -o "${GEN_DIR}"
        DEPENDS "${CMAKE_SOURCE_DIR}/tools/embed_assets.py" ${FRONTEND_FILES}
        COMMENT "Embedding Kutie frontend assets"
        VERBATIM
    )
    list(APPEND GENERATED_ASSET_SOURCES "${GEN_DIR}/asset_loader.cpp")
endif()

add_library(kutie STATIC
    src/asset_bundle.cpp
    src/ipc_hub.cpp
    src/runtime.cpp
    src/platform_services.cpp
    src/platform/shell_factory.cpp
    src/platform/windows/win_shell.cpp
    src/platform/windows/win_dialog.cpp
    src/platform/windows/win_dpi.cpp
    src/platform/windows/win_string_util.cpp
    ${GENERATED_ASSET_SOURCES}
)

target_compile_definitions(kutie PUBLIC KUTIE_PLATFORM_WINDOWS=1)
target_include_directories(kutie PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${WEBVIEW2_INCLUDE_DIR}"
    "${GEN_DIR}"
)

target_link_libraries(kutie PUBLIC
    nlohmann_json::nlohmann_json
    shlwapi
    shell32
    comdlg32
    ole32
    shcore
    dwmapi
    ${WEBVIEW2_LIBRARY}
)

set(SAMPLE_EXTRA_SOURCES "")
if(MSVC)
    list(APPEND SAMPLE_EXTRA_SOURCES "${CMAKE_BINARY_DIR}/generated/dpi_awareness.manifest")
endif()

add_executable(sample WIN32 sample/src/main.cpp ${SAMPLE_EXTRA_SOURCES})
if(Python3_Interpreter_FOUND)
    target_sources(sample PRIVATE "${GEN_DIR}/assets.rc")
endif()
target_link_libraries(sample PRIVATE kutie)

if(Python3_Interpreter_FOUND)
    add_custom_target(embed_assets DEPENDS "${GEN_DIR}/assets.rc" "${GEN_DIR}/asset_loader.cpp")
    add_dependencies(kutie embed_assets)
    add_dependencies(sample embed_assets)
endif()
