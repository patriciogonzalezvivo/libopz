add_executable (opz_dump opz_dump.cpp)

set_target_properties(opz_dump PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(opz_dump PRIVATE
    "${PROJECT_SOURCE_DIR}/deps"
    "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(opz_dump PRIVATE libopz)

