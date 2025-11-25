set(CORE_LIB_NAME "core" CACHE INTERNAL "")

add_library(${CORE_LIB_NAME} 
    "camera.hpp"
    "extras.hpp"
    "extras.cpp"
    "input.hpp"
    "mesh.hpp"
    "mesh.cpp"
    "models.hpp"
    "node.cpp"
    "node.hpp"
    "pipeline_factory.hpp"
    "renderer.hpp"
    "renderer_extras.hpp"
    "trail.cpp"
    "trail.hpp"
    "vertex.hpp"
    "vma_usage.cpp"
    "vulkan_renderer.cpp"
    "vulkan_renderer.hpp"
)

target_link_libraries(${CORE_LIB_NAME}
    PUBLIC 
        glfw 
        Vulkan::Vulkan 
        imgui::imgui 
        glm::glm 
    PRIVATE
        GPUOpen::VulkanMemoryAllocator
)

target_include_directories(${CORE_LIB_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/third_party/implot
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/third_party/stb_image
        ${CMAKE_SOURCE_DIR}/src/third_party/tiny_obj_loader
)

target_sources(${CORE_LIB_NAME} PUBLIC 
    ${CMAKE_SOURCE_DIR}/src/third_party/stb_image/stb_image.h
    ${CMAKE_SOURCE_DIR}/src/third_party/tiny_obj_loader/tiny_obj_loader.h
    ${CMAKE_SOURCE_DIR}/src/third_party/implot/implot.cpp
    ${CMAKE_SOURCE_DIR}/src/third_party/implot/implot_items.cpp
    ${CMAKE_SOURCE_DIR}/src/third_party/implot/implot_internal.h
    ${CMAKE_SOURCE_DIR}/src/third_party/implot/implot_demo.cpp
    ${CMAKE_SOURCE_DIR}/src/third_party/implot/implot.h
)