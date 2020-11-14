# fetch
if(NOT TARGET Catch2::Catch2)
    message("Including Catch2 !")
    Include(FetchContent)

    FetchContent_Declare(Catch2
        GIT_REPOSITORY "https://github.com/catchorg/Catch2"
        GIT_TAG "v2.x"
    )

    FetchContent_MakeAvailable(Catch2)
    FetchContent_GetProperties(Catch2
        SOURCE_DIR CATCH_SOURCE_DIR
    )
endif()

#add tests
add_executable(audiotube_tests tests.cpp)
target_link_libraries(audiotube_tests
    audiotube
    Catch2::Catch2
)

#tests handling
include(CTest)
list(APPEND CMAKE_MODULE_PATH ${CATCH_SOURCE_DIR}/contrib)
include(Catch)
catch_discover_tests(audiotube_tests)