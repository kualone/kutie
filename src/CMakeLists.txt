find_package(nlohmann_json CONFIG REQUIRED)

find_package(unofficial-webview2 CONFIG QUIET)
if(TARGET unofficial::webview2::webview2)
    set(KUTIE_WEBVIEW2_TARGET unofficial::webview2::webview2)
else()
    set(WEBVIEW2_SEARCH_PATHS
        "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-windows"
        "$ENV{VCPKG_ROOT}/installed/x64-windows"
    )
    find_path(WEBVIEW2_INCLUDE_DIR NAMES WebView2.h PATH_SUFFIXES include PATHS ${WEBVIEW2_SEARCH_PATHS})
    find_library(WEBVIEW2_LIBRARY NAMES WebView2LoaderStatic WebView2Loader.dll.lib
        PATH_SUFFIXES lib PATHS ${WEBVIEW2_SEARCH_PATHS})
    if(NOT WEBVIEW2_INCLUDE_DIR OR NOT WEBVIEW2_LIBRARY)
        message(FATAL_ERROR "WebView2 SDK not found. Install with: vcpkg install webview2:x64-windows")
    endif()
endif()

add_library(kutie STATIC
    asset_bundle.cpp
    ipc_hub.cpp
    runtime.cpp
    platform_services.cpp
    platform/shell_factory.cpp
    platform/windows/win_shell.cpp
    platform/windows/win_dialog.cpp
    platform/windows/win_dpi.cpp
    platform/windows/win_string_util.cpp
)

add_library(Kutie::kutie ALIAS kutie)

target_compile_definitions(kutie PUBLIC KUTIE_PLATFORM_WINDOWS=1)
target_include_directories(kutie PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(KUTIE_WEBVIEW2_TARGET)
    target_link_libraries(kutie PUBLIC
        nlohmann_json::nlohmann_json
        ${KUTIE_WEBVIEW2_TARGET}
        shlwapi
        shell32
        comdlg32
        ole32
        shcore
        dwmapi
    )
else()
    target_include_directories(kutie PUBLIC "${WEBVIEW2_INCLUDE_DIR}")
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
endif()

if(KUTIE_INSTALL)
    install(TARGETS kutie EXPORT KutieTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/kutie"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
