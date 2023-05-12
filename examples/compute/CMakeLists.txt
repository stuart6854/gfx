add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/compute.spv"
        COMMAND $ENV{VK_SDK_PATH}/Bin/dxc -T cs_6_0 -E "Main" -spirv -fvk-use-dx-layout -fspv-target-env=vulkan1.3 -Fo "${CMAKE_CURRENT_BINARY_DIR}/compute.spv" "compute.hlsl"
        DEPENDS "compute.hlsl"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Shaders."
)
add_custom_target(gfx_example_compute_shader DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/compute.spv")

add_executable(gfx_example_compute main.cpp)

target_link_libraries(gfx_example_compute PRIVATE gfx)

add_dependencies(gfx_example_compute gfx_example_compute_shader)