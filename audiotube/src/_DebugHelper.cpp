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

#include <spdlog/spdlog.h>

#include "_DebugHelper.h"

void AudioTube::DebugHelper::_dumpAsJSON(const UrlQuery &query) {
    auto dump = nlohmann::json::object();
    _fillJSON(query, &dump);
    _dumpAsJSON(dump);
}

void AudioTube::DebugHelper::_fillJSON(const UrlQuery &query, nlohmann::json * recRef) {
    // fill data
    if (!query.hasSubqueries()) {
        auto key = query.key();
        auto val = query.percentDecoded();
        (*recRef)[key] = val;
        return;
    }

    for (const auto &subq : query.subqueries()) {
        _fillJSON(subq, recRef);
    }
}

void AudioTube::DebugHelper::_dumpAsFile(const std::string &str) {
    std::ofstream fh;

    auto fullpath = std::filesystem::absolute("./yt.txt");
    fh.open(fullpath);
    fh << str;
    fh.close();

    spdlog::debug("[{}] created !", fullpath.string());
}

void AudioTube::DebugHelper::_dumpAsJSON(const nlohmann::json &doc) {
    std::ofstream fh;

    auto fullpath = std::filesystem::absolute("./yt.json");
    fh.open(fullpath);
    fh << doc.dump(4);
    fh.close();

    spdlog::debug("[{}] created !", fullpath.string());
}
