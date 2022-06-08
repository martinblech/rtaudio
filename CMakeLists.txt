cmake_minimum_required(VERSION 3.11)
cmake_policy(SET CMP0042 NEW)
set (CMAKE_CXX_STANDARD 11)
set (BUILD_SHARED_LIBS OFF)

project (rtaudio)

include(thirdparty/iir.cmake)
include(thirdparty/pffft.cmake)
include(thirdparty/portaudio.cmake)

include_directories(${CMAKE_JS_INC})
file(GLOB SOURCE_FILES CONFIGURE_DEPENDS "src/*.cc" "src/*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

target_link_libraries(${PROJECT_NAME} iir)
target_link_libraries(${PROJECT_NAME} pffft)
target_link_libraries(${PROJECT_NAME} ${PORTAUDIO_EXTRA_LIBS} portaudio)

# Include Node-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REGEX REPLACE "[\r\n\"]" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

# define NPI_VERSION
add_definitions(-DNAPI_VERSION=5)
