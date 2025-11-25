find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

add_definitions(
    -DGLM_FORCE_CTOR_INIT
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE
    -DGLM_FORCE_LEFT_HANDED
    -DGLM_FORCE_RADIANS
    -DGLM_FORCE_DEFAULT_ALIGNED_GENTYPES
    -DGLM_ENABLE_EXPERIMENTAL
)

add_subdirectory("core")
add_subdirectory("ray_tracing")
add_subdirectory("simulation")
