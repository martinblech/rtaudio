include(ExternalProject)
ExternalProject_Add(project_portaudio
    GIT_REPOSITORY      https://github.com/PortAudio/portaudio.git
    GIT_TAG             v19.7.0
    BUILD_COMMAND       make portaudio_static
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
endif (APPLE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(PORTAUDIO_EXTRA_LIBS rt asound jack pthread)
endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
