function(gfx_setup_dependencies)
    find_package(Vulkan REQUIRED)
    find_package(unofficial-vulkan-memory-allocator-hpp CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)
    find_package(tinyobjloader CONFIG REQUIRED)
endfunction()