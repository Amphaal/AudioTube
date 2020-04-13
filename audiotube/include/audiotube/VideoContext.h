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
#include <QRegularExpression>

#include "_DebugHelper.h"
#include "SignatureDecipherer.h"

class VideoContext {
    public:
        enum PreferedAudioStreamsInfosSource {
            Unknown,
            DASH, 
            UrlEncoded,
            JSON
        };
        using RawDashManifest = QString;
        
        using AudioStreamUrlByITag = QHash<uint, QUrl>;
        using AudioStreamsPackage = QPair<PreferedAudioStreamsInfosSource, AudioStreamUrlByITag>;
        VideoContext(
            const QString &playerSourceUrl, 
            const QString &dashManifestUrl, 
            const QString &adaptiveStreamInfosUrlEncoded, 
            const QJsonArray &adaptiveStreamInfosJson, 
            const QDateTime &validUntil
        );
    
    AudioStreamsPackage getUrlsByAudioStreams(const SignatureDecipherer* dcfrer) const;
    QString playerSourceUrl() const;

    //Dash manifest deciphering not handled yet ! //TODO
    QString decipherDashManifestUrl(const SignatureDecipherer* dcfrer) const;
    void fillRawDashManifest(const RawDashManifest &rawDashManifest);

    private:
        QString _playerSourceUrl;
        QString _dashManifestUrl;
        RawDashManifest _rawDashManifest;
        QJsonArray _adaptiveStreamInfosUrlEncoded;
        QJsonArray _adaptiveStreamInfosJson;
        QDateTime _validUntil;

        bool adaptiveStreamAsUrlEncoded = false;
        bool adaptiveStreamAsJson = false;
        bool adaptiveStreamAsDash = false;

        AudioStreamsPackage getUrlsByAudioStreams_UrlEncoded(const SignatureDecipherer* dcfrer) const;
        AudioStreamsPackage getUrlsByAudioStreams_DASH(const SignatureDecipherer* dcfrer) const;
        AudioStreamsPackage getUrlsByAudioStreams_JSON(const SignatureDecipherer* dcfrer) const;
        static bool _isCodecAllowed(const QString &codec);
        static bool _isMimeAllowed(const QString &mime);
        static QJsonArray _urlEncodedToJsonArray(const QString &urlQueryAsRawStr);

};