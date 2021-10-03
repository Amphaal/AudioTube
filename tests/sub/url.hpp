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

#include <audiotube/UrlParser.h>

#include <catch2/catch.hpp>

TEST_CASE("URL Parsing - Simple", "[URL]") {
  AudioTube::UrlParser up("https://www.youtube.com/watch?v=XarcApEC5ME");
  REQUIRE(up.scheme() == "https");
  REQUIRE(up.host() == "www.youtube.com");
  REQUIRE(up.pathAndQuery() == "/watch?v=XarcApEC5ME");
}

TEST_CASE("URL Parsing - single slash for path", "[URL]") {
  // single slash for path
  AudioTube::UrlParser up("https://www.google.com/");
  REQUIRE(up.scheme() == "https");
  REQUIRE(up.host() == "www.google.com");
  REQUIRE(up.pathAndQuery() == "/");
}

TEST_CASE("URL Parsing - no slash for path", "[URL]") {
  // no slash for path
  AudioTube::UrlParser up("https://www.google.com");
  REQUIRE(up.scheme() == "https");
  REQUIRE(up.host() == "www.google.com");
  REQUIRE(up.pathAndQuery() == "/");
}
