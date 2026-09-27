if(RAYTRACING_BUILD_TESTS)
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        # Match the project's Windows runtime, including clang targeting MSVC.
        set(gtest_force_shared_crt ON CACHE BOOL "Use the shared Windows CRT" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(googletest
            URL https://codeload.github.com/google/googletest/zip/refs/tags/v1.17.0
            URL_HASH SHA256=40d4ec942217dcc84a9ebe2a68584ada7d4a33a8ee958755763278ea1c5e18ff
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(googletest)
    endif()
    add_executable(raytracing_unit_tests
        sparse_set_tests.cpp physics_tests.cpp input_mesh_tests.cpp
        scene_tests.cpp simulation_tests.cpp
        ../src/simulation/src/simulation/particle.cpp
        ../src/simulation/src/simulation/simulation.cpp
        ../src/raytracing/src/raytracing/surface.cpp
    )
    target_link_libraries(raytracing_unit_tests PRIVATE raster GTest::gtest_main)
    target_include_directories(raytracing_unit_tests PRIVATE
        ${PROJECT_SOURCE_DIR}/src/simulation/include
        ${PROJECT_SOURCE_DIR}/src/raytracing/include
    )
    set_target_compile_options(raytracing_unit_tests)
    include(GoogleTest)
    gtest_discover_tests(raytracing_unit_tests
        DISCOVERY_MODE PRE_TEST
        DISCOVERY_TIMEOUT 60
        NO_PRETTY_VALUES
        PROPERTIES LABELS "unit" TIMEOUT 30
    )
endif()

if(RAYTRACING_BUILD_SMOKE_TESTS)
    add_executable(renderer_lifetime_smoke renderer_lifetime_smoke.cpp)
    target_link_libraries(renderer_lifetime_smoke PRIVATE raster)
    set_target_compile_options(renderer_lifetime_smoke)
    add_custom_command(TARGET renderer_lifetime_smoke POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/shaders/raster"
            "$<TARGET_FILE_DIR:renderer_lifetime_smoke>/shaders"
    )
    add_test(NAME renderer_lifetime_smoke COMMAND renderer_lifetime_smoke)
    set_tests_properties(renderer_lifetime_smoke PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:renderer_lifetime_smoke>"
        LABELS "gpu"
        TIMEOUT 60
        FAIL_REGULAR_EXPRESSION "Validation Error;VUID-"
    )
endif()
