SET(GFX_EXAMPLE_NAME "gfx_example_hello_triangle")

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/triangle.spv"
        COMMAND $ENV{VK_SDK_PATH}/Bin/dxc -T cs_6_0 -E "Main" -spirv -fvk-use-dx-layout -fspv-target-env=vulkan1.3 -Fo "${CMAKE_CURRENT_BINARY_DIR}/triangle.spv" "triangle.hlsl"
        DEPENDS "triangle.hlsl"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Shaders."
)
add_custom_target("${GFX_EXAMPLE_NAME}_shader" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/triangle.spv")

add_executable(${GFX_EXAMPLE_NAME} main.cpp)

target_link_libraries(${GFX_EXAMPLE_NAME} PRIVATE gfx glfw)

add_dependencies(${GFX_EXAMPLE_NAME} "${GFX_EXAMPLE_NAME}_shader")