SET(GFX_EXAMPLE_NAME "gfx_example_shader_uniforms")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/uniforms.vert.spv"
        COMMAND $ENV{VK_SDK_PATH}/Bin/dxc -T vs_6_0 -E "vs_main" -spirv -fvk-use-dx-layout -fspv-target-env=vulkan1.3 -Fo "${CMAKE_CURRENT_BINARY_DIR}/uniforms.vert.spv" "uniforms.hlsl"
        DEPENDS "uniforms.hlsl"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Shaders."
)
add_custom_target("${GFX_EXAMPLE_NAME}_shader_vert" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/uniforms.vert.spv")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/uniforms.frag.spv"
        COMMAND $ENV{VK_SDK_PATH}/Bin/dxc -T ps_6_0 -E "ps_main" -spirv -fvk-use-dx-layout -fspv-target-env=vulkan1.3 -Fo "${CMAKE_CURRENT_BINARY_DIR}/uniforms.frag.spv" "uniforms.hlsl"
        DEPENDS "uniforms.hlsl"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Shaders."
)
add_custom_target("${GFX_EXAMPLE_NAME}_shader_frag" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/uniforms.frag.spv")

add_executable(${GFX_EXAMPLE_NAME} main.cpp)

target_include_directories(gfx PRIVATE ../libs/include)
target_link_libraries(${GFX_EXAMPLE_NAME} PRIVATE gfx glfw)

add_dependencies(${GFX_EXAMPLE_NAME} "${GFX_EXAMPLE_NAME}_shader_vert")
add_dependencies(${GFX_EXAMPLE_NAME} "${GFX_EXAMPLE_NAME}_shader_frag")
