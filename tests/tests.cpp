#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <audiotube/YoutubeVideoMetadata.h>
#include <audiotube/YoutubeHelper.h>

bool youtube_metadata_fetching_succeeded(const QString &ytId) {
    auto container = YoutubeVideoMetadata::fromVideoId(ytId);
    auto defer = YoutubeHelper::refreshMetadata(&container);
    promise::all({defer});
    return !container.hasFailed();
}

TEST_CASE( "Fetch youtube video metadata", "[metadata]" ) {
    REQUIRE( youtube_metadata_fetching_succeeded("MnoajJelaAo") );
}