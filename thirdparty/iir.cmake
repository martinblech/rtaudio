include(ExternalProject)
ExternalProject_Add(project_iir
    GIT_REPOSITORY      https://github.com/berndporr/iir1.git
    GIT_TAG             6ef872dfe5e443decfa963d7e0f972a3826ea101
)
ExternalProject_Get_Property(project_iir BINARY_DIR)
ExternalProject_Get_Property(project_iir SOURCE_DIR)
set(iir_lib_dir "${BINARY_DIR}")
set(iir_inc_dir "${SOURCE_DIR}")
add_library(iir STATIC IMPORTED)
set_property(TARGET iir PROPERTY IMPORTED_LOCATION "${iir_lib_dir}/libiir_static.a")
include_directories(include ${iir_inc_dir})
target_link_libraries(${PROJECT_NAME} iir)
