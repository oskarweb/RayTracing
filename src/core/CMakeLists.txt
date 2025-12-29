set(CORE_LIB_NAME "core" CACHE INTERNAL "")

message(STATUS "Configuring core library: ${CORE_LIB_NAME}")

add_library(${CORE_LIB_NAME} STATIC
    "base_object.hpp"
    "camera.hpp"
    "extras.hpp"
    "extras.cpp"
    "imports.cpp"
    "input.hpp"
    "mesh.hpp"
    "mesh.cpp"
    "models.hpp"
    "node.cpp"
    "node.hpp"
    "pipeline_factory.hpp"
    "renderer.hpp"
    "renderer_extras.hpp"
    "sparse_set.hpp"
    "trail.cpp"
    "trail.hpp"
    "vertex.hpp"
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
)

add_library(implot STATIC
    ${CMAKE_SOURCE_DIR}/third_party/implot/implot.cpp
    ${CMAKE_SOURCE_DIR}/third_party/implot/implot_items.cpp
    ${CMAKE_SOURCE_DIR}/third_party/implot/implot_internal.h
    ${CMAKE_SOURCE_DIR}/third_party/implot/implot_demo.cpp
    ${CMAKE_SOURCE_DIR}/third_party/implot/implot.h
)
target_include_directories(implot SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/third_party/implot)
target_link_libraries(implot PRIVATE imgui::imgui)

add_library(stb_image INTERFACE ${CMAKE_SOURCE_DIR}/third_party/stb_image/stb_image.h)
target_include_directories(stb_image SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/third_party/stb_image)

add_library(tiny_obj_loader INTERFACE ${CMAKE_SOURCE_DIR}/third_party/tiny_obj_loader/tiny_obj_loader.h)
target_include_directories(tiny_obj_loader SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/third_party/tiny_obj_loader)

target_link_libraries(${CORE_LIB_NAME} 
    PUBLIC
        implot
        stb_image
        tiny_obj_loader
)

set_target_compile_options(${CORE_LIB_NAME})