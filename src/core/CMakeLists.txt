set(CORE_LIB_NAME "core" CACHE INTERNAL "")

message(STATUS "Configuring core library: ${CORE_LIB_NAME}")

add_library(${CORE_LIB_NAME} STATIC
    "include/core/resources/allocated_resources.hpp"
    "src/core/resources/allocated_resources.cpp"
    "include/core/vulkan/vulkan_handles.hpp"
    "include/core/vulkan/commands.hpp"
    "include/core/scene/base_object.hpp"
    "include/core/camera/camera.hpp"
    "include/core/extras.hpp"
    "src/core/extras.cpp"
    "src/core/resources/imports.cpp"
    "include/core/camera/input.hpp"
    "include/core/resources/mesh.hpp"
    "src/core/resources/mesh.cpp"
    "include/core/scene/models.hpp"
    "src/core/scene/node.cpp"
    "include/core/scene/node.hpp"
    "include/core/scene/renderer.hpp"
    "include/core/scene/renderer_extras.hpp"
    "include/core/sparse_set.hpp"
    "src/core/scene/trail.cpp"
    "include/core/scene/trail.hpp"
    "include/core/resources/vertex.hpp"
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
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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
