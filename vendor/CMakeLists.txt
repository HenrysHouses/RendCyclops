set(SDL_TEST OFF)
set(SDLTTF_HARFBUZZ OFF)  # Disable HarfBuzz
set(SDLTTF_VENDORED ON)
add_subdirectory(SDL)
add_subdirectory(SDL_image)
add_subdirectory(SDL_ttf)
add_subdirectory(clay)

add_library(vendor INTERFACE)
target_link_libraries(vendor INTERFACE SDL3_ttf::SDL3_ttf)
target_link_libraries(vendor INTERFACE SDL3::SDL3)
target_link_libraries(vendor INTERFACE SDL3_image::SDL3_image)
target_include_directories(vendor INTERFACE "clay")
