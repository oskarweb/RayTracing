cmake_minimum_required(VERSION 4.1)

project("RayTracing")

# Enable Hot Reload for MSVC compilers if supported.
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug>:EditAndContinue>,$<$<CONFIG:RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    message(STATUS "Configuring for MSVC")
    add_compile_options(/analyze)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /Zc:char8_t-")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-variable -Wno-unused-parameter)
    message(STATUS "Configuring for non-MSVC compiler")
endif()

add_subdirectory("src")
