# kutie_require_static_webview2()
# Fails configure when WebView2Loader is not statically linked.
function(kutie_require_static_webview2)
    if(NOT TARGET unofficial::webview2::webview2)
        message(FATAL_ERROR
            "Kutie requires unofficial::webview2::webview2. "
            "Install dependencies with vcpkg triplet x64-windows-static.")
    endif()

    get_target_property(_kutie_wv2_type unofficial::webview2::webview2 TYPE)
    if(NOT _kutie_wv2_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR
            "Kutie requires WebView2LoaderStatic (STATIC_LIBRARY). "
            "Rebuild vcpkg dependencies with triplet x64-windows-static.")
    endif()
endfunction()
