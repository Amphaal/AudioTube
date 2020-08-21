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

#pragma once

#include <audiotube/VideoMetadata.h>
#include <audiotube/NetworkFetcher.h>

#include <chrono>
#include <thread>
#include <string>

#include <catch2/catch.hpp>

bool youtube_metadata_fetching_succeeded(const std::string &ytId) {
    // generating container
    spdlog::set_level(spdlog::level::debug);
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
    return AudioTube::NetworkFetcher::isStreamAvailable(&container);
}

//
// Test cases
//

// TEST_CASE("Unavailable video", "[metadata]") {
//   REQUIRE_FALSE(youtube_metadata_fetching_succeeded("MnoajJelaAo"));
// }

TEST_CASE("OK from JSON Adaptative Stream - no deciphering", "[metadata]") {
  REQUIRE(youtube_metadata_fetching_succeeded("-Q5Y037vIyc"));
}

// TEST_CASE("OK from Dash Manifest - no url deciphering", "[metadata]") {
//   REQUIRE(youtube_metadata_fetching_succeeded("qyYFF3Eh6lw"));
// }

// TEST_CASE("Restricted viewing", "[metadata]") {
//   REQUIRE(youtube_metadata_fetching_succeeded("dNv1ImIa1-4"));
// }

// TEST_CASE("Exact STS required", "[metadata]") {
//   REQUIRE(youtube_metadata_fetching_succeeded("lkkHtuTdIj4"));
// }
