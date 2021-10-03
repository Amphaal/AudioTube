// AudioTube C++
// C++ fork based on https://github.com/Tyrrrz/YoutubeExplode
// Copyright (C) 2019-2021 Guillaume Vara

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <spdlog/spdlog.h>

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

// TEST_CASE("Decipherer test", "[metadata]") {
//     spdlog::set_level(spdlog::level::debug);
//     std::string source   = "kOq0QJ8wRgIhAICjNBeVlp_19y5PthHrq2Vy4vkcVEUAgZwDLno0z6CGAiEAj7MmsGvh_nxApH_X3pFSPURqC3btePsjUYQLgaTAI88=88=";
//     std::string expected = "AOq0QJ8wRgIhAICjNBeVlp_19y5PthHrq2Vy4vkcVEUkgZwDLno0z6CGAiEAj7MmsGvh_nxApH_X3pFSPURqC3btePsjUYQLgaTAI88=";

//     AudioTube::SignatureDecipherer::YTDecipheringOperations operations;
//     operations.push({ AudioTube::CipherOperation::Reverse, 0 });
//     operations.push({ AudioTube::CipherOperation::Slice, 3 });
//     operations.push({ AudioTube::CipherOperation::Reverse, 0 });
//     operations.push({ AudioTube::CipherOperation::Swap, 43 });

//     auto cipherer = AudioTube::SignatureDecipherer(operations);
//     auto deciphered = cipherer.decipher(source);

//     REQUIRE(deciphered == expected);
// }

TEST_CASE("Unavailable video", "[metadata]") {
    REQUIRE_FALSE(youtube_metadata_fetching_succeeded("MnoajJelaAo"));
}

TEST_CASE("OK from JSON Adaptative Stream - no deciphering", "[metadata]") {
    REQUIRE(youtube_metadata_fetching_succeeded("-Q5Y037vIyc"));
}

TEST_CASE("OK from Player Response - url deciphering", "[metadata]") {
    REQUIRE(youtube_metadata_fetching_succeeded("qyYFF3Eh6lw"));
}

TEST_CASE("Restricted viewing - NOK from Embed, OK from Watch Page", "[metadata]") {
    REQUIRE(youtube_metadata_fetching_succeeded("dNv1ImIa1-4"));
}

TEST_CASE("Exact STS required - No DASH Manifest", "[metadata]") {
    REQUIRE(youtube_metadata_fetching_succeeded("lkkHtuTdIj4"));
}
