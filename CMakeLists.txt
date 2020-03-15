#policies first
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0048 NEW)
endif()

#declare project
cmake_minimum_required(VERSION 3.15.4)
project(AudioTube
    VERSION 0.0.1
    DESCRIPTION "Request Youtube API to get audio streams"
    HOMEPAGE_URL "https://github.com/Amphaal/AudioTube"
    LANGUAGES CXX
)

#cpp standards
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

#find Qt5 Network
find_package(Qt5 REQUIRED COMPONENTS
    Core 
    Network 
)

#Qt moc 
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON) 

#create as static lib
add_library(audiotube STATIC)

target_link_libraries(audiotube 
    Qt5::Network 
    Qt5::Core
)

#add deps
include_directories("promise-cpp")
target_sources(audiotube PUBLIC
    "promise-cpp/promise.hpp"
)

#header interface
file(GLOB_RECURSE H_FILES "audiotube/*.h")
target_sources(audiotube PUBLIC 
    ${H_FILES}
)
target_include_directories(audiotube INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

#add source files
file(GLOB_RECURSE CPP_FILES "audiotube/*.cpp")
target_sources(audiotube PUBLIC
    ${CPP_FILES}
)