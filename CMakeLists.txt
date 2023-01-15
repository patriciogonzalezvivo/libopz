cmake_minimum_required(VERSION 3.1)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 1)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

project(libopz  
        VERSION ${VERSION}
        LANGUAGES CXX )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The compiled library code is here
add_subdirectory(deps)
add_subdirectory(src)

target_compile_definitions(libopz PRIVATE LIBOPZ_VERSION_MAJOR=${VERSION_MAJOR})
target_compile_definitions(libopz PRIVATE LIBOPZ_VERSION_MINOR=${VERSION_MINOR})
target_compile_definitions(libopz PRIVATE LIBOPZ_VERSION_PATCH=${VERSION_PATCH})

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_subdirectory(examples)
endif()