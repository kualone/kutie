cmake_minimum_required(VERSION 3.20)
project(Kutie VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static CRT (/MT) for single-exe distribution without VC++ Redistributable.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    option(KUTIE_BUILD_SAMPLES "Build the Kutie sample application" ON)
else()
    option(KUTIE_BUILD_SAMPLES "Build the Kutie sample application" OFF)
endif()
option(KUTIE_INSTALL "Install Kutie CMake package and library" ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(KUTIE_DPI_MANIFEST_CONTENT "<?xml version='1.0' encoding='UTF-8' standalone='yes'?>
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

# Single-exe distribution: static CRT + static WebView2Loader via vcpkg triplet.
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "vcpkg triplet (static CRT and WebView2LoaderStatic)")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
endif()

set(_KUTIE_PACKAGE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "Kutie package root for embed helper")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/KutieEmbedAssets.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/KutieWindows.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/KutieStaticLinking.cmake")

add_subdirectory(src)

if(KUTIE_BUILD_SAMPLES)
    add_subdirectory(sample)
endif()

if(KUTIE_INSTALL)
    install(EXPORT KutieTargets
        FILE KutieTargets.cmake
        NAMESPACE Kutie::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Kutie"
    )

    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/KutieEmbedAssets.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/KutieWindows.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/KutieStaticLinking.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Kutie"
    )

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_assets.py"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/kutie/tools"
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/KutieConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/KutieConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Kutie"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/KutieConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/KutieConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/KutieConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Kutie"
    )
endif()
