get_filename_component(_KUTIE_LOCAL_SOURCE "${CURRENT_PORT_DIR}/../.." ABSOLUTE)

if(EXISTS "${_KUTIE_LOCAL_SOURCE}/CMakeLists.txt")
    set(SOURCE_PATH "${_KUTIE_LOCAL_SOURCE}")
else()
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO kunge/kutie
        REF "v${VERSION}"
        SHA512 0
        HEAD_REF main
    )
endif()

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DKUTIE_BUILD_SAMPLES=OFF
        -DKUTIE_INSTALL=ON
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME Kutie CONFIG_PATH lib/cmake/Kutie)
vcpkg_copy_pdbs()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
