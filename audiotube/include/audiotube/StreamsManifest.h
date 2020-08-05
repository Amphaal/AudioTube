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

#include <algorithm>
#include <functional>
#include <utility>
#include <string>
#include <map>
#include <unordered_map>
#include <regex>
#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "_DebugHelper.h"
#include "_NetworkHelper.h"
#include "SignatureDecipherer.h"
#include "Regexes.h"

namespace AudioTube {

class StreamsManifest : public NetworkHelper {
 public:
    enum AudioStreamsSource {
        PlayerResponse,
        PlayerConfig,
        DASH
    };

    using RawDASHManifest = std::string;
    using RawPlayerConfigStreams = std::string;
    using AudioStreamUrl = std::string;
    using RawPlayerResponseStreams = nlohmann::json;
    using ITag = int;

    using AudioStreamUrlByBitrate = std::map<double, std::pair<ITag, AudioStreamUrl>>;
    using AudioStreamsPackage = std::map<AudioStreamsSource, AudioStreamUrlByBitrate>;

    StreamsManifest();

    // TODO(amphaal) add deciphering
    void feedRaw_DASH(const RawDASHManifest &raw, const SignatureDecipherer* decipherer);
    void feedRaw_PlayerConfig(const RawPlayerConfigStreams &raw, const SignatureDecipherer* decipherer);
    void feedRaw_PlayerResponse(const RawPlayerResponseStreams &raw, const SignatureDecipherer* decipherer);

    void setRequestedAt(const std::time_t &requestedAt);
    void setSecondsUntilExpiration(const unsigned int secsUntilExp);

    std::pair<StreamsManifest::AudioStreamsSource, AudioStreamUrlByBitrate> preferedStreamSource() const;
    std::string preferedUrl() const;
    bool isExpired() const;

 private:
    std::time_t _requestedAt = -1;
    std::time_t _validUntil = -1;

    AudioStreamsPackage _package;

    static bool _isCodecAllowed(const std::string &codec);
    static bool _isMimeAllowed(const std::string &mime);

    static std::string _decipheredUrl(const SignatureDecipherer* decipherer, const std::string &cipheredUrl, std::string signature, std::string sigKey = std::string());
};

}  // namespace AudioTube
