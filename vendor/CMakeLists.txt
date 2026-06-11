set(SDL_TEST OFF)
add_subdirectory(SDL)
add_subdirectory(SDL_image)
# add_subdirectory(glad)

add_library(vendor INTERFACE)
target_link_libraries(vendor INTERFACE SDL3::SDL3)
target_link_libraries(vendor INTERFACE SDL3_image::SDL3_image)
# target_link_libraries(vendor INTERFACE glad)
