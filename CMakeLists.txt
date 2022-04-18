cmake_minimum_required(VERSION 3.1)

set(VERSION 0.1)
project(opz     VERSION ${VERSION}
                LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(deps)

file(GLOB ROOT_SOURCE "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(opz
    ${ROOT_SOURCE}
)

target_include_directories(opz PRIVATE
    deps
)

target_link_libraries(opz PRIVATE
    rtmidi
)

install(TARGETS opz
        RUNTIME DESTINATION bin)
