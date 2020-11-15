#policies first
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0048 NEW)
endif()

#declare project
cmake_minimum_required(VERSION 3.10.2) #stick to 18.04 Ubuntu LTS version
project(AudioTube
    VERSION 0.2.3
    DESCRIPTION "Request API to get audio streams"
    HOMEPAGE_URL "https://github.com/Amphaal/AudioTube"
    LANGUAGES CXX
)

#cpp standards
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

###################
## Deps : spdlog ##
###################

if(NOT TARGET spdlog::spdlog)
    #try to find it in packages
    find_package(spdlog)

    # if not found, then fetch it from source !
    if(NOT spdlog_FOUND)
        message("Including [spdlog] !")
            Include(FetchContent)
            FetchContent_Declare(spdlog
                GIT_REPOSITORY "https://github.com/gabime/spdlog"
                GIT_TAG "v1.x"
            )
            FetchContent_MakeAvailable(spdlog)
    endif()
endif()

##
##
##

add_subdirectory(audiotube)

#dont build tests if included as submodule
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
