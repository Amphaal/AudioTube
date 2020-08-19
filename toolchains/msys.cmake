SET(CMAKE_BUILD_TYPE Debug) 

SET(CMAKE_RANLIB                    "llvm-ranlib")
SET(CMAKE_AR                        "llvm-ar")

SET (CMAKE_C_COMPILER               "clang")
SET (CMAKE_C_FLAGS                  "-fuse-ld=lld -Wall")
SET (CMAKE_C_FLAGS_DEBUG            "-O0 -g")

SET (CMAKE_CXX_COMPILER             "clang++")
SET (CMAKE_CXX_FLAGS                "-fuse-ld=lld -Wall")
SET (CMAKE_CXX_FLAGS_DEBUG          "-O0 -g")