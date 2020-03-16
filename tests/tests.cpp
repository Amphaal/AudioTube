#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <audiotube/YoutubeVideoMetadata.h>
#include <audiotube/YoutubeHelper.h>

#include <chrono>
#include <thread>

bool youtube_metadata_fetching_succeeded(const QString &ytId) {
    auto container = YoutubeVideoMetadata::fromVideoId(ytId);
    YoutubeHelper::refreshMetadata(&container).then([]() { return promise::resolve(); });
    while(!container.ranOnce()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return !container.hasFailed();
}

TEST_CASE( "Fetch youtube video metadata", "[metadata]" ) {
    REQUIRE( youtube_metadata_fetching_succeeded("MnoajJelaAo") );
}