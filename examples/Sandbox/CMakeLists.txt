add_executable(gfx_example_sandbox main.cpp)

target_link_libraries(gfx_example_sandbox PRIVATE gfx)
target_link_libraries(gfx_example_sandbox PRIVATE glm::glm)
target_link_libraries(gfx_example_sandbox PRIVATE glfw)
