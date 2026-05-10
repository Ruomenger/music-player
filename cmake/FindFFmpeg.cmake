#[=======================================================================[.rst:
FindFFmpeg
----------

Find FFmpeg libraries (avcodec, avformat, avutil, swresample).

Imported targets
^^^^^^^^^^^^^^^^

``FFmpeg::avcodec``, ``FFmpeg::avformat``, ``FFmpeg::avutil``, ``FFmpeg::swresample``

Result variables
^^^^^^^^^^^^^^^^

``FFMPEG_FOUND``, ``FFMPEG_INCLUDE_DIRS``, ``FFMPEG_LIBRARIES``
#]=======================================================================]

# If targets already exist (e.g., from vcpkg), don't override
if(TARGET FFmpeg::avcodec)
    return()
endif()

include(FindPackageHandleStandardArgs)

if(NOT FFMPEG_INCLUDE_DIR)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(_PC_FFMPEG QUIET
            libavcodec libavformat libavutil libswresample)
    endif()
endif()

if(_PC_FFMPEG_FOUND)
    set(FFMPEG_INCLUDE_DIRS ${_PC_FFMPEG_INCLUDE_DIRS})
    set(FFMPEG_LIBRARY_DIRS ${_PC_FFMPEG_LIBRARY_DIRS})
    set(FFMPEG_LIBRARIES    ${_PC_FFMPEG_LIBRARIES})
    set(FFMPEG_VERSION      ${_PC_FFMPEG_VERSION})

    find_path(FFMPEG_INCLUDE_DIR NAMES libavcodec/avcodec.h
              HINTS ${_PC_FFMPEG_INCLUDE_DIRS})

    macro(_find_ffmpeg_lib name)
        find_library(FFmpeg_${name}_LIBRARY
            NAMES ${name}
            HINTS ${_PC_FFMPEG_LIBRARY_DIRS})
    endmacro()
else()
    find_path(FFMPEG_INCLUDE_DIR NAMES libavcodec/avcodec.h
              PATH_SUFFIXES ffmpeg)

    macro(_find_ffmpeg_lib name)
        find_library(FFmpeg_${name}_LIBRARY NAMES ${name})
    endmacro()
endif()

# Required components — find them all
set(_ffmpeg_components avcodec avformat avutil swresample)
foreach(_comp ${_ffmpeg_components})
    _find_ffmpeg_lib(${_comp})
endforeach()

# Mark per-component found (supports find_package COMPONENTS)
foreach(_comp ${FFmpeg_FIND_COMPONENTS})
    if(FFmpeg_${_comp}_LIBRARY)
        set(FFmpeg_${_comp}_FOUND TRUE)
    endif()
endforeach()

find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS
        FFmpeg_avcodec_LIBRARY
        FFmpeg_avformat_LIBRARY
        FFmpeg_avutil_LIBRARY
        FFmpeg_swresample_LIBRARY
        FFMPEG_INCLUDE_DIR
    VERSION_VAR FFMPEG_VERSION
    HANDLE_COMPONENTS
)

if(FFmpeg_FOUND AND NOT TARGET FFmpeg::avcodec)
    macro(_create_ffmpeg_target name)
        if(NOT TARGET FFmpeg::${name})
            add_library(FFmpeg::${name} UNKNOWN IMPORTED)
            set_target_properties(FFmpeg::${name} PROPERTIES
                IMPORTED_LOCATION "${FFmpeg_${name}_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
            )
        endif()
    endmacro()

    _create_ffmpeg_target(avcodec)
    _create_ffmpeg_target(avformat)
    _create_ffmpeg_target(avutil)
    _create_ffmpeg_target(swresample)

    set_target_properties(FFmpeg::avformat PROPERTIES
        INTERFACE_LINK_LIBRARIES "FFmpeg::avcodec;FFmpeg::avutil"
    )
    set_target_properties(FFmpeg::avcodec PROPERTIES
        INTERFACE_LINK_LIBRARIES "FFmpeg::avutil"
    )
    set_target_properties(FFmpeg::swresample PROPERTIES
        INTERFACE_LINK_LIBRARIES "FFmpeg::avutil"
    )

    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
    set(FFMPEG_LIBRARIES
        ${FFmpeg_avformat_LIBRARY}
        ${FFmpeg_avcodec_LIBRARY}
        ${FFmpeg_avutil_LIBRARY}
        ${FFmpeg_swresample_LIBRARY}
    )
endif()

mark_as_advanced(
    FFMPEG_INCLUDE_DIR
    FFmpeg_avcodec_LIBRARY
    FFmpeg_avformat_LIBRARY
    FFmpeg_avutil_LIBRARY
    FFmpeg_swresample_LIBRARY
)