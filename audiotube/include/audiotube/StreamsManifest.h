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

#include <QString>
#include <QJsonArray>
#include <QDateTime>
#include <QJsonObject>
#include <QVariantHash>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>
#include <functional>

#include "_DebugHelper.h"
#include "_NetworkHelper.h"
#include "SignatureDecipherer.h"
#include "Regexes.h"

class StreamsManifest : public NetworkHelper {
    Q_GADGET

 public:
    enum AudioStreamsSource {
        PlayerResponse,
        PlayerConfig,
        DASH
    };
    Q_ENUM(AudioStreamsSource)

    using RawDASHManifest = QString;
    using RawPlayerConfigStreams = QString;
    using RawPlayerResponseStreams = QJsonArray;
    using ITag = int;

    using AudioStreamUrlByBitrate = QMap<double, QPair<ITag, QUrl>>;
    using AudioStreamsPackage = QHash<AudioStreamsSource, AudioStreamUrlByBitrate>;

    StreamsManifest();

    // TODO(amphaal) add deciphering
    void feedRaw_DASH(const RawDASHManifest &raw, const SignatureDecipherer* decipherer);
    void feedRaw_PlayerConfig(const RawPlayerConfigStreams &raw, const SignatureDecipherer* decipherer);
    void feedRaw_PlayerResponse(const RawPlayerResponseStreams &raw, const SignatureDecipherer* decipherer);

    void setRequestedAt(const QDateTime &requestedAt);
    void setSecondsUntilExpiration(const uint secsUntilExp);

    QPair<StreamsManifest::AudioStreamsSource, AudioStreamUrlByBitrate> preferedStreamSource() const;
    QUrl preferedUrl() const;
    bool isExpired() const;

 private:
    QDateTime _requestedAt;
    QDateTime _validUntil;

    AudioStreamsPackage _package;

    static bool _isCodecAllowed(const QString &codec);
    static bool _isMimeAllowed(const QString &mime);
    static QJsonArray _urlEncodedToJsonArray(const QString &urlQueryAsRawStr);

    static QString _decipheredUrl(const SignatureDecipherer* decipherer, const QString &cipheredUrl, QString signature, QString sigKey = QString());
};

inline uint qHash(const StreamsManifest::AudioStreamsSource &key, uint seed = 0) {return uint(key) ^ seed;}
