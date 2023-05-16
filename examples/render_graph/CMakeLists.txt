SET(GFX_EXAMPLE_NAME "gfx_example_render_graph")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/model.vert.spv"
        COMMAND $ENV{VK_SDK_PATH}/Bin/dxc -T vs_6_0 -E "vs_main" -spirv -fvk-use-dx-layout -fspv-target-env=vulkan1.3 -Fo "${CMAKE_CURRENT_BINARY_DIR}/model.vert.spv" "model.hlsl"
        DEPENDS "model.hlsl"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Shaders."
)
add_custom_target("${GFX_EXAMPLE_NAME}_shader_vert" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/model.vert.spv")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/model.frag.spv"
        COMMAND $ENV{VK_SDK_PATH}/Bin/dxc -T ps_6_0 -E "ps_main" -spirv -fvk-use-dx-layout -fspv-target-env=vulkan1.3 -Fo "${CMAKE_CURRENT_BINARY_DIR}/model.frag.spv" "model.hlsl"
        DEPENDS "model.hlsl"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Shaders."
)
add_custom_target("${GFX_EXAMPLE_NAME}_shader_frag" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/model.frag.spv")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/stanford-bunny.obj"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "stanford-bunny.obj" "${CMAKE_CURRENT_BINARY_DIR}/stanford-bunny.obj"
        DEPENDS "stanford-bunny.obj"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Copying model."
)
add_custom_target("${GFX_EXAMPLE_NAME}_model" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/stanford-bunny.obj")

add_executable(${GFX_EXAMPLE_NAME} main.cpp)

target_link_libraries(${GFX_EXAMPLE_NAME} PRIVATE gfx glfw glm::glm tinyobjloader::tinyobjloader)

add_dependencies(${GFX_EXAMPLE_NAME} "${GFX_EXAMPLE_NAME}_shader_vert")
add_dependencies(${GFX_EXAMPLE_NAME} "${GFX_EXAMPLE_NAME}_shader_frag")
add_dependencies(${GFX_EXAMPLE_NAME} "${GFX_EXAMPLE_NAME}_model")
