add_executable(hiromi_example main.cpp)

target_link_libraries(hiromi_example PRIVATE HiromiEngine)

# Copy assets to build output directory so the example can find shaders/textures
add_custom_command(TARGET hiromi_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:hiromi_example>/assets
    COMMENT "Copying assets to build directory"
)
