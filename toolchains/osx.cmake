SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON) #prevent linking errors once app shared
SET(CMAKE_MACOSX_RPATH ON)

#set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk)
SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)

list(APPEND CMAKE_PREFIX_PATH 
    "/usr/local/opt"
    "/usr/local/opt/qt"
)

set(OPENSSL_ROOT_DIR "/usr/local/Cellar/openssl@1.1/1.1.1f")

SET(CMAKE_BUILD_TYPE Debug)