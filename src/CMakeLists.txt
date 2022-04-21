file(GLOB ROOT_SOURCE 
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_library(libopz
    ${ROOT_SOURCE}
)

set_target_properties(libopz PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CMAKE_CXX_EXTENSIONS OFF
)

target_include_directories(libopz PRIVATE
    "${PROJECT_SOURCE_DIR}/deps"
)

find_package(ZLIB)

target_link_libraries(libopz PRIVATE
    rtmidi
    ZLIB::ZLIB
)
