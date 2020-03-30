#policies first
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0048 NEW)
endif()

#declare project
cmake_minimum_required(VERSION 3.17.0)
project(AudioTube
    VERSION 0.0.1
    DESCRIPTION "Request  API to get audio streams"
    HOMEPAGE_URL "https://github.com/Amphaal/AudioTube"
    LANGUAGES CXX
)

#cpp standards
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(audiotube)

#dont build tests if included as submodule
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()