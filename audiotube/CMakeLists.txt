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
    src/Regexes.cpp
)

########################
## Deps : promise-cpp ##
########################

add_library(promise-cpp INTERFACE)
target_include_directories(promise-cpp INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/promise-cpp")

#link
target_link_libraries(audiotube PUBLIC promise-cpp)

###################
## Deps : spdlog ##
###################

#link
target_link_libraries(audiotube PRIVATE spdlog::spdlog)

###########################
## Deps : asio + OpenSSL ##
###########################

find_package(OpenSSL REQUIRED)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include")
target_link_libraries(asio INTERFACE OpenSSL::Crypto OpenSSL::SSL)
IF(WIN32)
    target_compile_definitions(asio INTERFACE WIN32_LEAN_AND_MEAN) # prevents <winsock2.h> warnings
    target_link_libraries(asio INTERFACE ws2_32 mswsock)
endif()

target_link_libraries(audiotube PUBLIC asio)

##########################
## Deps : nlohmann_json ##
##########################

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json/single_include")

target_link_libraries(audiotube PUBLIC nlohmann_json)

############################
## Deps : PCRE2 + wrapper ##
############################

SET(PCRE2_BUILD_PCRE2GREP OFF CACHE BOOL "" FORCE)
SET(PCRE2_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(pcre2 EXCLUDE_FROM_ALL)
target_include_directories(pcre2-8 INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/pcre2")

add_library(jpcre2 INTERFACE)
target_include_directories(jpcre2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/jpcre2/src")
target_link_libraries(jpcre2 INTERFACE pcre2-8)
target_link_libraries(audiotube PUBLIC jpcre2)
