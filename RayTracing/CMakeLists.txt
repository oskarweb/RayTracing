set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows/share")

add_executable(RayTracing 
    "extras.hpp"     
    "VulkanRenderer.cpp" 
    "VulkanRenderer.hpp" 
    "Simulation.cpp"
    "Simulation.hpp"
    "Camera.hpp"
    "main.cpp" 
    "PipelineFactory.hpp"
    "Vertex.hpp" 
    "Mesh.hpp"
    "Particle.cpp"
    "Particle.hpp"
    "RendererStructs.hpp"
    "Input.hpp"
    "Models.hpp"
    "Node.cpp"
    "Node.hpp"
    "Renderer.hpp"
    "Trail.cpp" 
    "Trail.hpp"
    "VmaUsage.cpp"
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:RelWithDebInfo>:/Od /Zi /UNDEBUG>
)      

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

target_link_libraries(RayTracing PRIVATE
    glfw Vulkan::Vulkan
    imgui::imgui
    glm::glm
    GPUOpen::VulkanMemoryAllocator
)

target_include_directories(RayTracing PRIVATE 
    # "third_party/stb_image"
    # "third_party/tiny_obj_loader"
    # "third_party/implot"
)

target_sources(RayTracing PRIVATE 
    # "third_party/stb_image/stb_image.h"
    # "third_party/tiny_obj_loader/tiny_obj_loader.h"
    # "third_party/implot/implot.cpp"
    # "third_party/implot/implot_items.cpp"
    # "third_party/implot/implot_internal.h"
    # "third_party/implot/implot_demo.cpp"
    # "third_party/implot/implot.h"
)

add_custom_command(
    TARGET RayTracing POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/RayTracing/fonts" "$<TARGET_FILE_DIR:RayTracing>/fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/RayTracing/models" "$<TARGET_FILE_DIR:RayTracing>/models"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/RayTracing/shaders" "$<TARGET_FILE_DIR:RayTracing>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/RayTracing/textures" "$<TARGET_FILE_DIR:RayTracing>/textures"
)