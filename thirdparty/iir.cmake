include(ExternalProject)
ExternalProject_Add(project_iir
    GIT_REPOSITORY      https://github.com/berndporr/iir1.git
    GIT_TAG             c6516bdfc96c9af40075e45ebd45c9396b6a5e44
    BUILD_COMMAND       make iir_static
    INSTALL_COMMAND     :
)
ExternalProject_Get_Property(project_iir BINARY_DIR)
ExternalProject_Get_Property(project_iir SOURCE_DIR)
set(iir_lib_dir "${BINARY_DIR}")
set(iir_inc_dir "${SOURCE_DIR}")
add_library(iir STATIC IMPORTED)
set_property(TARGET iir PROPERTY IMPORTED_LOCATION "${iir_lib_dir}/libiir_static.a")
include_directories(include ${iir_inc_dir})
