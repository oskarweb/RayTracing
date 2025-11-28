function(make_system target)
    get_target_property(dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
    if(dirs)
        set_target_properties(${target} PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${dirs}"
        )
    endif()
endfunction()

function(set_target_compile_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /analyze /analyze:external-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-variable -Wno-unused-parameter -Wno-reorder)
    endif()
endfunction()