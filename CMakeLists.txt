cmake_minimum_required(VERSION 3.24)
project(RendCyclops LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(vendor)
add_subdirectory(src)

# target_compile_options(RendCyclops
#     PRIVATE
#         -Wall
#         -Werror
# )

add_subdirectory(vendor/glad/cmake glad_cmake)
glad_add_library(glad_gl STATIC API gl:core=3.3)
target_link_libraries(RendCyclops PRIVATE glad_gl ${SDL3_LIBRARIES})

add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        ${CMAKE_CURRENT_BINARY_DIR}/resources
)
add_dependencies(RendCyclops copy_resources)
