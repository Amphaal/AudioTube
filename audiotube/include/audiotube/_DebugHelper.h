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

#include <string>
#include <fstream>
#include <filesystem>

#include "UrlParser.h"

#include <nlohmann/json.hpp>

namespace AudioTube {

class DebugHelper {
 public:
    static void _dumpAsJSON(const UrlQuery &query);
    static void _dumpAsJSON(const nlohmann::json &doc);
    static void _dumpAsFile(const std::string &str);

 private:
    static void _fillJSON(const UrlQuery &query, nlohmann::json * recRef);
};

}  // namespace AudioTube
