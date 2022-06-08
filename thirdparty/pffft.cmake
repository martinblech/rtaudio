include(FetchContent)
FetchContent_Declare(project_pffft
    GIT_REPOSITORY      https://bitbucket.org/jpommier/pffft.git
    GIT_TAG             7c3b5a7dc510a0f513b9c5b6dc5b56f7aeeda422
)
FetchContent_MakeAvailable(project_pffft)
add_library(pffft SHARED 
  "${project_pffft_SOURCE_DIR}/fftpack.h"
  "${project_pffft_SOURCE_DIR}/fftpack.c"
  "${project_pffft_SOURCE_DIR}/pffft.h"
  "${project_pffft_SOURCE_DIR}/pffft.c"
)
include_directories(include ${project_pffft_SOURCE_DIR})
