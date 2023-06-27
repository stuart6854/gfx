add_library(gfx gfx.cpp gfx_render_graph.cpp)

target_include_directories(gfx PUBLIC ../includes PRIVATE ../libs/include)

target_link_directories(gfx PUBLIC ../libs/lib)
target_link_libraries(gfx PUBLIC Vulkan::Vulkan)