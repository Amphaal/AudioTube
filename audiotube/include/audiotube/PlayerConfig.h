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

#include "ATHelper.h"
#include "_NetworkHelper.h"
#include "SignatureDecipherer.h"
#include "StreamsManifest.h"

#include <nlohmann/json.hpp>

namespace AudioTube {

class PlayerConfig : public NetworkHelper {
 public:
    enum ContextSource {
        Unknown,
        EmbedPage,
        WatchPage
    };

    using VideoId = std::string;

    static promise::Promise from_EmbedPage(const PlayerConfig::VideoId &videoId);
    static promise::Promise from_WatchPage(const PlayerConfig::VideoId &videoId, StreamsManifest* streamsManifest);

    PlayerConfig();

    std::string sts() const;
    int duration() const;
    std::string title() const;
    const SignatureDecipherer* decipherer() const;
    ContextSource contextSource() const;

    void fillFromVideoInfosDetails(const std::string &title, int duration);

 private:
    PlayerConfig(const ContextSource &streamContextSource, const PlayerConfig::VideoId &videoId);

    ContextSource _contextSource = ContextSource::Unknown;
    SignatureDecipherer* _decipherer = nullptr;
    std::string _title;
    int _duration = 0;
    std::string _sts;

    static promise::Promise _downloadRaw_VideoEmbedPageHtml(const PlayerConfig::VideoId &videoId);
    static promise::Promise _downloadRaw_WatchPageHtml(const PlayerConfig::VideoId &videoId);

    promise::Promise _downloadAndfillFrom_PlayerSource(const std::string &playerSourceUrl);

    promise::Promise _fillFrom_WatchPageHtml(const DownloadedUtf8 &dl, StreamsManifest* streamsManifest);
    promise::Promise _fillFrom_VideoEmbedPageHtml(const DownloadedUtf8 &dl);
    promise::Promise _fillFrom_PlayerSource(const DownloadedUtf8 &dl, const std::string &playerSourceUrl);

    static nlohmann::json _extractPlayerConfigFromRawSource(const DownloadedUtf8 &rawSource, const jp::Regex &regexToJSONStart);
    static std::string _extractPlayerSourceURLFromRawSource(const DownloadedUtf8 &rawSource);

    // extraction helpers
    static std::string _playerSourceUrl(const nlohmann::json &playerConfig);
    static std::string _getSts(const DownloadedUtf8 &dl);
};

}  // namespace AudioTube
