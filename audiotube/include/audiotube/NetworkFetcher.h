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

#include <QList>

#include <QDebug>

#include <QRegularExpression>
#include <QUrlQuery>
#include <QString>
#include <QFile>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "_NetworkHelper.h"
#include "_DebugHelper.h"

#include "PlayerConfiguration.h"
#include "VideoMetadata.h"

class NetworkFetcher : public NetworkHelper {
    
    public:
    
    //workaround for promise::all bug...
    struct DataHolder {
        QString playerSourceUrl;
        QString title;
        int duration;
        QDateTime expirationDate;
        QString dashManifestUrl;
        QString streamInfos_UrlEncoded;
        QJsonArray streamInfos_JSON;
    };

        static promise::Defer fromPlaylistUrl(const QString &url);
        static promise::Defer refreshMetadata(VideoMetadata* toRefresh, bool force = false);
        static void isStreamAvailable(VideoMetadata* toCheck, bool* checkEnded = nullptr, QString* urlSuccessfullyRequested = nullptr);

    private:
        static promise::Defer _getVideoEmbedPageHtml(const VideoMetadata::Id &videoId);
        static promise::Defer _getWatchPageHtml(const VideoMetadata::Id &videoId);
        static promise::Defer _getVideoInfosDic(const VideoMetadata::Id &videoId, const QString &sts);

        static promise::Defer _getStreamContext(VideoMetadata* metadata);
        static promise::Defer _getStreamContext_VideoInfo(VideoMetadata* metadata);
        static promise::Defer _getStreamContext_WatchPage(VideoMetadata* metadata);

        static promise::Defer _extractDataFrom_VideoInfos(const DownloadedUtf8 &dl, const QDateTime &requestedAt);
        static promise::Defer _extractDataFrom_EmbedPageHtml(const DownloadedUtf8 &videoEmbedPageRequestData);
        static promise::Defer _extractDataFrom_WatchPage(const DownloadedUtf8 &dl, const QDateTime &requestedAt);

        static QString _extractPlayerSourceUrlFromPlayerConfig(const QJsonObject &playerConfig);
        static QJsonObject _extractPlayerConfigFromRawSource(const DownloadedUtf8 &rawSource, const QRegularExpression &regex);

        static promise::Defer _extractDeciphererAndStsFromPlayerSource(PlayerConfiguration &playerConfig);

        static promise::Defer _mayFillDashManifestXml(PlayerConfiguration &playerConfig, const SignatureDecipherer* decipherer);

        static QList<QString> _extractVideoIdsFromHTTPRequest(const DownloadedUtf8 &requestData);
        static QList<VideoMetadata*> _videoIdsToMetadataList(const QList<QString> &videoIds);
};