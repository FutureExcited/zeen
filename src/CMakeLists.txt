file(GLOB_RECURSE ZEEN_SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)
file(GLOB_RECURSE HLSL_FILES CONFIGURE_DEPENDS assets/*.hlsl assets/*.hlsli)
set_source_files_properties(${HLSL_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(zeen ${ZEEN_SOURCES} ${HLSL_FILES})
set_target_properties(zeen PROPERTIES MACOSX_BUNDLE TRUE)
target_include_directories(zeen PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(zeen PRIVATE core toolkit)

be_apply_defaults(zeen)

# Copy zeen's own assets (shaders + per-lobby assets) next to the binary.
add_custom_command(TARGET zeen POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:zeen>/assets")

be_copy_runtime_deps(zeen)

if(APPLE)
    add_custom_command(TARGET zeen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:zeen>/Contents/Resources/vulkan/icd.d"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/MoltenVK_icd.json"
            "$<TARGET_BUNDLE_DIR:zeen>/Contents/Resources/vulkan/icd.d/MoltenVK_icd.json")
endif()
