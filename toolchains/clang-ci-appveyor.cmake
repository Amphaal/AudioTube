SET(CMAKE_BUILD_TYPE Release)
SET(Qt5_DIR "C:/Qt/5.14/msvc2017_64/lib/cmake/Qt5") 

SET (CMAKE_C_COMPILER               "clang")
SET (CMAKE_C_FLAGS                  "-fuse-ld=lld -Wall")
SET (CMAKE_C_FLAGS_RELEASE          "-O3 -DNDEBUG")

# SET (CMAKE_EXE_LINKER_FLAGS_INIT    "-Xlinker /implib:libcmt.lib")
# SET (CMAKE_SHARED_LINKER_FLAGS_INIT ${CMAKE_EXE_LINKER_FLAGS_INIT})

SET (CMAKE_CXX_COMPILER             "clang++")
SET (CMAKE_CXX_FLAGS                "-fuse-ld=lld -Wall")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")