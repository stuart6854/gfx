add_library(gfx gfx.cpp gfx_render_graph.cpp)

target_include_directories(gfx PUBLIC ../includes)

target_link_libraries(gfx PUBLIC Vulkan::Vulkan unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)