include(FetchContent)
FetchContent_Declare(project_pffft
    GIT_REPOSITORY      https://bitbucket.org/jpommier/pffft.git
    GIT_TAG             2b2bd45bbf9be04fd22ece5cc1f54679202e9257
)
FetchContent_MakeAvailable(project_pffft)
add_library(pffft SHARED 
  "${project_pffft_SOURCE_DIR}/fftpack.h"
  "${project_pffft_SOURCE_DIR}/fftpack.c"
  "${project_pffft_SOURCE_DIR}/pffft.h"
  "${project_pffft_SOURCE_DIR}/pffft.c"
)
include_directories(include ${project_pffft_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} pffft)
