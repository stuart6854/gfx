add_library(gfx gfx.cpp)

target_include_directories(gfx PUBLIC ../includes)

target_link_libraries(gfx PUBLIC Vulkan::Vulkan unofficial::VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)