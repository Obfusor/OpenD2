# FindAllegro5.cmake
# Finds Allegro 5 library on Linux/macOS/ARM
#
# Sets:
#   ALLEGRO5_FOUND       - True if Allegro 5 was found
#   ALLEGRO5_INCLUDE_DIR - Include directory for Allegro 5 headers
#   ALLEGRO5_LIBRARIES   - Libraries to link against

# Try pkg-config first (most Linux distros)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(ALLEGRO5_PC QUIET allegro-5 allegro_image-5 allegro_font-5
        allegro_ttf-5 allegro_primitives-5 allegro_audio-5 allegro_acodec-5)
endif()

if(ALLEGRO5_PC_FOUND)
    set(ALLEGRO5_INCLUDE_DIR ${ALLEGRO5_PC_INCLUDE_DIRS})
    set(ALLEGRO5_LIBRARIES ${ALLEGRO5_PC_LIBRARIES})
    set(ALLEGRO5_FOUND TRUE)
else()
    # Manual search
    find_path(ALLEGRO5_INCLUDE_DIR allegro5/allegro.h
        HINTS
            /usr/include
            /usr/local/include
            /opt/local/include
    )

    find_library(ALLEGRO5_CORE_LIB NAMES allegro allegro-5.2
        HINTS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
    )
    find_library(ALLEGRO5_IMAGE_LIB NAMES allegro_image allegro_image-5.2
        HINTS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
    )
    find_library(ALLEGRO5_FONT_LIB NAMES allegro_font allegro_font-5.2
        HINTS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
    )
    find_library(ALLEGRO5_TTF_LIB NAMES allegro_ttf allegro_ttf-5.2
        HINTS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
    )
    find_library(ALLEGRO5_PRIMITIVES_LIB NAMES allegro_primitives allegro_primitives-5.2
        HINTS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
    )
    find_library(ALLEGRO5_AUDIO_LIB NAMES allegro_audio allegro_audio-5.2
        HINTS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
    )
    find_library(ALLEGRO5_ACODEC_LIB NAMES allegro_acodec allegro_acodec-5.2
        HINTS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
    )

    if(ALLEGRO5_CORE_LIB AND ALLEGRO5_IMAGE_LIB AND ALLEGRO5_FONT_LIB AND ALLEGRO5_INCLUDE_DIR)
        set(ALLEGRO5_LIBRARIES
            ${ALLEGRO5_CORE_LIB}
            ${ALLEGRO5_IMAGE_LIB}
            ${ALLEGRO5_FONT_LIB}
            ${ALLEGRO5_TTF_LIB}
            ${ALLEGRO5_PRIMITIVES_LIB}
            ${ALLEGRO5_AUDIO_LIB}
            ${ALLEGRO5_ACODEC_LIB}
        )
        set(ALLEGRO5_FOUND TRUE)
    else()
        set(ALLEGRO5_FOUND FALSE)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Allegro5
    REQUIRED_VARS ALLEGRO5_LIBRARIES ALLEGRO5_INCLUDE_DIR
)

mark_as_advanced(ALLEGRO5_INCLUDE_DIR ALLEGRO5_LIBRARIES)
