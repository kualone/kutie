# kutie_embed_frontend(target frontend_dir)
# Generates embedded asset registrar + Windows .rc and links them into the executable.
function(kutie_embed_frontend target frontend_dir)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "kutie_embed_frontend: target '${target}' does not exist")
    endif()
    if(NOT IS_DIRECTORY "${frontend_dir}")
        message(FATAL_ERROR "kutie_embed_frontend: frontend directory not found: ${frontend_dir}")
    endif()
    if(NOT _KUTIE_PACKAGE_ROOT)
        message(FATAL_ERROR "kutie_embed_frontend: _KUTIE_PACKAGE_ROOT is not set")
    endif()

    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(_kutie_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/kutie-embed-${target}")
    set(_kutie_embed_script "${_KUTIE_PACKAGE_ROOT}/tools/embed_assets.py")
    file(GLOB_RECURSE _kutie_frontend_files CONFIGURE_DEPENDS "${frontend_dir}/*")

    add_custom_command(
        OUTPUT
            "${_kutie_gen_dir}/assets.rc"
            "${_kutie_gen_dir}/kutie_embedded_assets.cpp"
            "${_kutie_gen_dir}/asset_ids.h"
        COMMAND ${Python3_EXECUTABLE} "${_kutie_embed_script}"
                "${frontend_dir}" -o "${_kutie_gen_dir}"
        DEPENDS "${_kutie_embed_script}" ${_kutie_frontend_files}
        COMMENT "Embedding Kutie frontend assets for ${target}"
        VERBATIM
    )

    target_sources(${target} PRIVATE
        "${_kutie_gen_dir}/kutie_embedded_assets.cpp"
        "${_kutie_gen_dir}/assets.rc"
    )
    target_include_directories(${target} PRIVATE "${_kutie_gen_dir}")

    add_custom_target(kutie-embed-${target} DEPENDS
        "${_kutie_gen_dir}/assets.rc"
        "${_kutie_gen_dir}/kutie_embedded_assets.cpp"
        "${_kutie_gen_dir}/asset_ids.h"
    )
    add_dependencies(${target} kutie-embed-${target})
endfunction()
