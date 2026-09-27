add_library(raster STATIC
    src/raster/raster_renderer.cpp
    include/raster/raster_renderer.hpp
    src/raster/pipeline_factory.cpp
    include/raster/pipeline_factory.hpp
)
target_link_libraries(raster PUBLIC ${CORE_LIB_NAME} PRIVATE GPUOpen::VulkanMemoryAllocator)
target_include_directories(raster PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_compile_options(raster)
