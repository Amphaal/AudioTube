// AudioTube C++
// C++ fork based on https://github.com/Tyrrrz/YoutubeExplode
// Copyright (C) 2019-2020 Guillaume Vara

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <audiotube/VideoMetadata.h>
#include <audiotube/NetworkFetcher.h>
#include <audiotube/_NetworkHelper.h>

#include <chrono>
#include <thread>
#include <future>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

bool stream_are_working(AudioTube::VideoMetadata &metadata) {
    // fetch for a stream HEAD
    std::string urlSuccessfullyRequested;
    bool ended;
    AudioTube::NetworkFetcher::isStreamAvailable(&metadata, &ended, &urlSuccessfullyRequested);

    // wait processing
    while (!ended) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // check url
    if (!urlSuccessfullyRequested.empty()) {
      // spdlog::debug("Stream URL found : {}", urlSuccessfullyRequested);
      return true;
    } else {
        return false;
    }
}

bool youtube_metadata_fetching_succeeded(const std::string &ytId) {
    // generating container
    AudioTube::VideoMetadata container(ytId, AudioTube::VideoMetadata::InstantiationType::InstFromId);
    spdlog::debug("Testing [{}]...", container.url());

    // refresh...
    AudioTube::NetworkFetcher::refreshMetadata(&container);

    // wait for a response
    while (!container.ranOnce()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // check if it has failed
    if (container.hasFailed()) return false;

    // check if a stream is working
    return stream_are_working(container);
}

//
// Test cases
//

TEST_CASE("Fetch HTTP HEAD", "[network]") {
  //TODO
}

TEST_CASE("URL Parsing", "[URL]") {
  //TODO
}

TEST_CASE("Unavailable video", "[metadata]") {
  REQUIRE_FALSE(youtube_metadata_fetching_succeeded("MnoajJelaAo"));
}

TEST_CASE("OK from JSON Adaptative Stream - no deciphering", "[metadata]") {
  REQUIRE(youtube_metadata_fetching_succeeded("-Q5Y037vIyc"));
}

TEST_CASE("OK from Dash Manifest - no url deciphering", "[metadata]") {
  REQUIRE(youtube_metadata_fetching_succeeded("qyYFF3Eh6lw"));
}

TEST_CASE("Restricted viewing", "[metadata]") {
  REQUIRE(youtube_metadata_fetching_succeeded("dNv1ImIa1-4"));
}

TEST_CASE("Exact STS required", "[metadata]") {
  REQUIRE(youtube_metadata_fetching_succeeded("lkkHtuTdIj4"));
}
