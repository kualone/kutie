add_executable(kutie_smoke_frameless smoke_frameless.cpp)
target_link_libraries(kutie_smoke_frameless PRIVATE kutie)
target_include_directories(kutie_smoke_frameless PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(kutie_smoke_browser_window smoke_browser_window.cpp)
target_link_libraries(kutie_smoke_browser_window PRIVATE kutie)
target_include_directories(kutie_smoke_browser_window PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME smoke_frameless COMMAND kutie_smoke_frameless)
add_test(NAME smoke_browser_window COMMAND kutie_smoke_browser_window)
