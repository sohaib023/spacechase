# Per-compiler warning flags, applied only to our own targets (never to
# third-party dependencies). clang-cl reports COMPILER_ID == "Clang" but uses
# the MSVC-style driver, so we key off the front-end variant, not the id.
#
# Toggle warnings-as-errors with -DSPACECHASE_WERROR=OFF if a dependency bump
# starts tripping warnings during a build.

option(SPACECHASE_WERROR "Treat warnings as errors on Space Chase targets" ON)

function(spacechase_set_warnings target)
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # Covers both MSVC and clang-cl.
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(SPACECHASE_WERROR)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        # GCC / Clang (GNU driver) — Linux & macOS.
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(SPACECHASE_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
