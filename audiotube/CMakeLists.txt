#Qt moc 
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON) 

#find Qt5
find_package(Qt5 REQUIRED COMPONENTS
    Core 
    Network
)

#create as static lib
add_library(audiotube STATIC)

target_link_libraries(audiotube 
    Qt5::Network 
    Qt5::Core
)

target_include_directories(audiotube
    PRIVATE include/audiotube
    INTERFACE .
    INTERFACE include
)

#add submodule sources
target_sources(audiotube PRIVATE
    promise-cpp/promise.hpp
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
)
