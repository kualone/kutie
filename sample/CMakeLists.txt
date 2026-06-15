add_executable(sample WIN32 src/main.cpp)
target_link_libraries(sample PRIVATE Kutie::kutie)

kutie_embed_frontend(sample "${CMAKE_CURRENT_SOURCE_DIR}/frontend")
kutie_apply_windows_manifest(sample)
