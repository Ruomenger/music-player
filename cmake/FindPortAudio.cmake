#[=======================================================================[.rst:
FindPortAudio
-------------

Find the PortAudio library.

Imported targets
^^^^^^^^^^^^^^^^

``PortAudio::PortAudio``
  The PortAudio library, if found.

Result variables
^^^^^^^^^^^^^^^^

``PortAudio_FOUND``
``PortAudio_INCLUDE_DIRS``
``PortAudio_LIBRARIES``

#]=======================================================================]

find_path(PortAudio_INCLUDE_DIR NAMES portaudio.h)

find_library(PortAudio_LIBRARY NAMES portaudio)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PortAudio
    REQUIRED_VARS PortAudio_LIBRARY PortAudio_INCLUDE_DIR)

if(PortAudio_FOUND AND NOT TARGET PortAudio::PortAudio)
    set(PortAudio_INCLUDE_DIRS ${PortAudio_INCLUDE_DIR})
    set(PortAudio_LIBRARIES ${PortAudio_LIBRARY})
    add_library(PortAudio::PortAudio UNKNOWN IMPORTED)
    set_target_properties(PortAudio::PortAudio PROPERTIES
        IMPORTED_LOCATION "${PortAudio_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PortAudio_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(PortAudio_INCLUDE_DIR PortAudio_LIBRARY)
