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

#include "_DebugHelper.h"
#include "_NetworkHelper.h"
#include "SignatureDecipherer.h"
#include "Regexes.h"

namespace AudioTube {

class StreamsManifest : public NetworkHelper {
    Q_GADGET

 public:
    enum AudioStreamsSource {
        PlayerResponse,
        PlayerConfig,
        DASH
    };
    Q_ENUM(AudioStreamsSource)

    using RawDASHManifest = std::string;
    using RawPlayerConfigStreams = std::string;
    using AudioStreamUrl = std::string;
    using RawPlayerResponseStreams = QJsonArray;
    using ITag = int;

    using AudioStreamUrlByBitrate = QMap<double, std::pair<ITag, AudioStreamUrl>>;
    using AudioStreamsPackage = QHash<AudioStreamsSource, AudioStreamUrlByBitrate>;

    StreamsManifest();

    // TODO(amphaal) add deciphering
    void feedRaw_DASH(const RawDASHManifest &raw, const SignatureDecipherer* decipherer);
    void feedRaw_PlayerConfig(const RawPlayerConfigStreams &raw, const SignatureDecipherer* decipherer);
    void feedRaw_PlayerResponse(const RawPlayerResponseStreams &raw, const SignatureDecipherer* decipherer);

    void setRequestedAt(const QDateTime &requestedAt);
    void setSecondsUntilExpiration(const unsigned int secsUntilExp);

    std::pair<StreamsManifest::AudioStreamsSource, AudioStreamUrlByBitrate> preferedStreamSource() const;
    QUrl preferedUrl() const;
    bool isExpired() const;

 private:
    QDateTime _requestedAt;
    QDateTime _validUntil;

    AudioStreamsPackage _package;

    static bool _isCodecAllowed(const std::string &codec);
    static bool _isMimeAllowed(const std::string &mime);
    static QJsonArray _urlEncodedToJsonArray(const std::string &urlQueryAsRawStr);

    static std::string _decipheredUrl(const SignatureDecipherer* decipherer, const std::string &cipheredUrl, std::string signature, std::string sigKey = std::string());
};

inline unsigned int qHash(const AudioTube::StreamsManifest::AudioStreamsSource &key, unsigned int seed = 0) {return unsigned int(key) ^ seed;}

}  // namespace AudioTube
