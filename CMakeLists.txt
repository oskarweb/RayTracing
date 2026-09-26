include(${CMAKE_SOURCE_DIR}/cmake/utils.cmake)

cmake_minimum_required(VERSION 4.1)

project("RayTracing")

# Enable Hot Reload for MSVC compilers if supported.
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug>:EditAndContinue>,$<$<CONFIG:RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory("src")

option(RAYTRACING_BUILD_TESTS "Build GoogleTest unit tests (no display required)" OFF)
option(RAYTRACING_BUILD_SMOKE_TESTS "Build GPU smoke tests (requires a Vulkan-capable display)" OFF)
if(RAYTRACING_BUILD_TESTS OR RAYTRACING_BUILD_SMOKE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
