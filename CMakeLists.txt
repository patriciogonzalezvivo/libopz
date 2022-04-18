cmake_minimum_required(VERSION 3.1)

set(VERSION 0.1)
project(opz     VERSION ${VERSION}
                LANGUAGES CXX)

# The compiled library code is here
add_subdirectory(deps)
add_subdirectory(src)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_subdirectory(opz_dump)
endif()