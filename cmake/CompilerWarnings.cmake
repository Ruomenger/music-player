if(NOT TARGET compiler_warnings)
    add_library(compiler_warnings INTERFACE)

    if(MSVC)
        target_compile_options(compiler_warnings INTERFACE /W4 /permissive-)
    else()
        target_compile_options(compiler_warnings INTERFACE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
            -Wimplicit-fallthrough
        )
    endif()
endif()
