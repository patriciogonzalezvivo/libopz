set(INCLUDE_FOLDER "${PROJECT_SOURCE_DIR}/include")
set(SOURCE_FOLDER "${PROJECT_SOURCE_DIR}/src")

file(GLOB LIBOPZ_HEADER  "${INCLUDE_FOLDER}/libopz/*.h")
file(GLOB LIBOPZ_SOURCE  "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_library(libopz ${LIBOPZ_SOURCE})

set_target_properties(libopz PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CMAKE_CXX_EXTENSIONS OFF
)

target_include_directories(libopz PRIVATE
    ${INCLUDE_FOLDER}
    "${PROJECT_SOURCE_DIR}/deps"
)

find_package(ZLIB)

target_link_libraries(libopz PRIVATE
    rtmidi
    ZLIB::ZLIB
)
