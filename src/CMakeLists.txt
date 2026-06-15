find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-webview2 CONFIG REQUIRED)
kutie_require_static_webview2()

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

target_link_libraries(kutie PUBLIC
    nlohmann_json::nlohmann_json
    unofficial::webview2::webview2
    shlwapi
    shell32
    comdlg32
    ole32
    shcore
    dwmapi
)

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
