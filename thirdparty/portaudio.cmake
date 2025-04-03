include(ExternalProject)
ExternalProject_Add(project_portaudio
    # TODO: Revert to official portaudio after upstream fix.
    GIT_REPOSITORY      https://github.com/martinblech/portaudio.git
    GIT_TAG             967d68edd5a93f0f8be4037727a84c67cccf367a
    BUILD_COMMAND       make portaudio
    INSTALL_COMMAND     :
)
ExternalProject_Get_Property(project_portaudio BINARY_DIR)
ExternalProject_Get_Property(project_portaudio SOURCE_DIR)
set(portaudio_lib_dir "${BINARY_DIR}")
set(portaudio_inc_dir "${SOURCE_DIR}/include")
add_library(portaudio STATIC IMPORTED)
set_property(TARGET portaudio PROPERTY IMPORTED_LOCATION "${portaudio_lib_dir}/libportaudio.a")
include_directories(include ${portaudio_inc_dir})
set(PORTAUDIO_EXTRA_LIBS)
if(APPLE)
   find_library(CORE_AUDIO_LIBRARY CoreAudio)
   set(PORTAUDIO_EXTRA_LIBS ${CORE_AUDIO_LIBRARY})
endif()
if(UNIX AND NOT APPLE)
  set(PORTAUDIO_EXTRA_LIBS rt asound pthread)
endif()
