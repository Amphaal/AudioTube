#create as static lib
add_library(audiotube STATIC)

target_link_libraries(audiotube 
    Qt5::Network 
    Qt5::Core
)

target_include_directories(audiotube 
    PUBLIC ${CMAKE_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

#add source files
target_sources(audiotube PRIVATE
    promise-cpp/promise.hpp
    NetworkFetcher.cpp
    SignatureDecipherer.cpp
    VideoMetadata.cpp
    _base/_NetworkHelper.cpp
    _base/_DebugHelper.cpp
)