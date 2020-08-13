#create as static lib
add_library(audiotube STATIC)

target_include_directories(audiotube
    PRIVATE include/audiotube
    INTERFACE .
    INTERFACE include
)

#headers for MOC
target_sources(audiotube PRIVATE
    include/audiotube/VideoMetadata.h
)

#force headers for MOC
target_sources(audiotube PRIVATE
    include/audiotube/StreamsManifest.h
)

#add lib files
target_sources(audiotube PRIVATE
    src/NetworkFetcher.cpp
    src/SignatureDecipherer.cpp
    src/VideoInfos.cpp
    src/PlayerConfig.cpp
    src/StreamsManifest.cpp
    src/VideoMetadata.cpp
    src/\_NetworkHelper.cpp
    src/\_DebugHelper.cpp
    src/UrlParser.cpp
    src/ATHelper.cpp
)

########################
## Deps : promise-cpp ##
########################

add_library(promise-cpp INTERFACE IMPORTED)
target_include_directories(promise-cpp INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/promise-cpp")

#link
target_link_libraries(audiotube PUBLIC promise-cpp)

###################
## Deps : spdlog ##
###################

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include")

#link
target_link_libraries(audiotube PUBLIC spdlog)

#################
## Deps : asio ##
#################

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include")

target_link_libraries(audiotube PUBLIC asio)

##########################
## Deps : nlohmann_json ##
##########################

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json/single_include")

target_link_libraries(audiotube PUBLIC nlohmann_json)

###############################
## Deps : compile time regex ##
###############################

add_library(ct_regex INTERFACE)
target_include_directories(ct_regex INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/ct_regex/single-header")

target_link_libraries(audiotube PUBLIC ct_regex)