# OPZ DUMP
add_executable (opz_dump opz_dump.cpp)
set_target_properties(opz_dump PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(opz_dump PRIVATE
    "${PROJECT_SOURCE_DIR}/deps"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(opz_dump PRIVATE libopz)

# OPZ COMPANION
add_executable (opz_companion opz_companion.cpp)
set_target_properties(opz_companion PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(opz_companion PRIVATE
    "${PROJECT_SOURCE_DIR}/deps"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(opz_companion PRIVATE libopz ncurses)

# OPZ TRACK EXPLORER
add_executable (opz_tracks opz_tracks.cpp)
set_target_properties(opz_tracks PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(opz_tracks PRIVATE
    "${PROJECT_SOURCE_DIR}/deps"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(opz_tracks PRIVATE libopz ncurses)