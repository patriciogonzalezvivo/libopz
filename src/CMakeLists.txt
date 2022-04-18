file(GLOB ROOT_SOURCE "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_library(opz
    ${ROOT_SOURCE}
)

set_target_properties(opz PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CMAKE_CXX_EXTENSIONS OFF
)

target_include_directories(opz PRIVATE
    "${PROJECT_SOURCE_DIR}/deps"
)

target_link_libraries(opz PRIVATE
    rtmidi
)
