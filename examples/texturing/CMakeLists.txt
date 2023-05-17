SET(GFX_EXAMPLE_NAME "gfx_example_texturing")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/texturing.vert.spv"
        COMMAND $ENV{VK_SDK_PATH}/Bin/dxc -T vs_6_0 -E "vs_main" -spirv -fvk-use-dx-layout -fspv-target-env=vulkan1.3 -Fo "${CMAKE_CURRENT_BINARY_DIR}/texturing.vert.spv" "texturing.hlsl"
        DEPENDS "texturing.hlsl"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Shaders."
)
add_custom_target("${GFX_EXAMPLE_NAME}_shader_vert" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/texturing.vert.spv")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/texturing.frag.spv"
        COMMAND $ENV{VK_SDK_PATH}/Bin/dxc -T ps_6_0 -E "ps_main" -spirv -fvk-use-dx-layout -fspv-target-env=vulkan1.3 -Fo "${CMAKE_CURRENT_BINARY_DIR}/texturing.frag.spv" "texturing.hlsl"
        DEPENDS "texturing.hlsl"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Shaders."
)
add_custom_target("${GFX_EXAMPLE_NAME}_shader_frag" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/texturing.frag.spv")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/viking_room.obj"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "viking_room.obj" "${CMAKE_CURRENT_BINARY_DIR}/viking_room.obj"
        DEPENDS "viking_room.obj"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Copying model."
)
add_custom_target("${GFX_EXAMPLE_NAME}_model" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/viking_room.obj")

add_executable(${GFX_EXAMPLE_NAME} main.cpp)

target_link_libraries(${GFX_EXAMPLE_NAME} PRIVATE gfx glfw glm::glm tinyobjloader::tinyobjloader)

add_dependencies(${GFX_EXAMPLE_NAME} "${GFX_EXAMPLE_NAME}_shader_vert")
add_dependencies(${GFX_EXAMPLE_NAME} "${GFX_EXAMPLE_NAME}_shader_frag")
add_dependencies(${GFX_EXAMPLE_NAME} "${GFX_EXAMPLE_NAME}_model")
