add_library(raster STATIC
    raster_renderer.cpp
    raster_renderer.hpp
    pipeline_factory.cpp
    pipeline_factory.hpp
)
target_link_libraries(raster PUBLIC ${CORE_LIB_NAME} PRIVATE GPUOpen::VulkanMemoryAllocator)
set_target_compile_options(raster)
