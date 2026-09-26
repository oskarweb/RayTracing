set(CORE_LIB_NAME "core" CACHE INTERNAL "")

message(STATUS "Configuring core library: ${CORE_LIB_NAME}")

add_library(${CORE_LIB_NAME} STATIC
    "resources/allocated_resources.hpp"
    "resources/allocated_resources.cpp"
    "vulkan/vulkan_handles.hpp"
    "vulkan/commands.hpp"
    "scene/base_object.hpp"
    "camera/camera.hpp"
    "extras.hpp"
    "extras.cpp"
    "resources/imports.cpp"
    "camera/input.hpp"
    "resources/mesh.hpp"
    "resources/mesh.cpp"
    "scene/models.hpp"
    "scene/node.cpp"
    "scene/node.hpp"
    "scene/renderer.hpp"
    "scene/renderer_extras.hpp"
    "sparse_set.hpp"
    "scene/trail.cpp"
    "scene/trail.hpp"
    "resources/vertex.hpp"
)

target_compile_definitions(${CORE_LIB_NAME} PUBLIC
    GLFW_INCLUDE_VULKAN
    GLM_FORCE_CTOR_INIT
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_LEFT_HANDED
    GLM_FORCE_RADIANS
    GLM_ENABLE_EXPERIMENTAL
)

target_compile_features(${CORE_LIB_NAME} PUBLIC cxx_std_23)

target_link_libraries(${CORE_LIB_NAME}
    PUBLIC 
        glfw
        Vulkan::Vulkan 
        imgui::imgui 
	implot::implot
        glm::glm 
    PRIVATE
        GPUOpen::VulkanMemoryAllocator
)

target_include_directories(${CORE_LIB_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

add_library(stb_image INTERFACE ${CMAKE_SOURCE_DIR}/third_party/stb_image/stb_image.h)
target_include_directories(stb_image SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/third_party/stb_image)

add_library(tiny_obj_loader INTERFACE ${CMAKE_SOURCE_DIR}/third_party/tiny_obj_loader/tiny_obj_loader.h)
target_include_directories(tiny_obj_loader SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/third_party/tiny_obj_loader)

target_link_libraries(${CORE_LIB_NAME} 
    PUBLIC
        stb_image
        tiny_obj_loader
)

set_target_compile_options(${CORE_LIB_NAME})
