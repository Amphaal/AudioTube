SET(CMAKE_BUILD_TYPE Debug)

SET (CMAKE_CXX_FLAGS "-Wno-deprecated-declarations") #prevent asio's std::allocator<void> depreciation warnings

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON) #prevent linking errors once app shared
SET(CMAKE_MACOSX_RPATH ON)
SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)

list(APPEND CMAKE_PREFIX_PATH 
    "/usr/local/opt"
)

set(OPENSSL_ROOT_DIR "/usr/local/Cellar/openssl@3/*")