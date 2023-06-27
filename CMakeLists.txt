cmake_minimum_required(VERSION 3.25)

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif ()

if (NOT DEFINED CMAKE_CXX_EXTENSIONS)
    # strongly encouraged to enable this globally to avoid conflicts between
    # -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
    # when compiling with PCH enabled
    set(CMAKE_CXX_EXTENSIONS OFF)
endif ()

# Set the project name and language
project(GFX LANGUAGES CXX)

include(Dependencies.cmake)
gfx_setup_dependencies()

add_subdirectory(src)

#if (gfx_ENABLE_EXAMPLES)
add_subdirectory(examples)
#endif ()