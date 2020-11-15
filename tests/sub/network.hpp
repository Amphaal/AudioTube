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

#include <audiotube/_NetworkHelper.h>

#include <string>

#include <catch2/catch.hpp>

TEST_CASE("Fetch HTTP HEAD", "[network]") {
  std::string sourceUrl = "https://ccadb-public.secure.force.com/mozilla/IncludedCACertificateReportCSVFormat";
  auto response = AudioTube::NetworkHelper::downloadHTTPS(sourceUrl, true);
}
