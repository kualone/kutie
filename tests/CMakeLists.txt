add_executable(kutie_smoke_frameless smoke_frameless.cpp)
target_link_libraries(kutie_smoke_frameless PRIVATE kutie)
target_include_directories(kutie_smoke_frameless PRIVATE "${CMAKE_SOURCE_DIR}/src")

add_test(NAME smoke_frameless COMMAND kutie_smoke_frameless)
