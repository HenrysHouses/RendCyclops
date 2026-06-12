file(GLOB_RECURSE SOURCES "*.c" "*.h")
add_executable(${PROJECT_NAME} ${SOURCES})
# target_sources(${PROJECT_NAME} PRIVATE main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE vendor)
