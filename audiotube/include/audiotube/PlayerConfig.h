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

#include "_NetworkHelper.h"
#include "VideoMetadata.h"
#include "SignatureDecipherer.h"

class PlayerConfig : public NetworkHelper {
    public:
        static promise::Defer from(const VideoMetadata::PreferedStreamContextSource &streamContextSource, const VideoMetadata::Id &videoId);

        QString videoInfoUrl() const;

    private:
        PlayerConfig(const VideoMetadata::PreferedStreamContextSource &streamContextSource, const VideoMetadata::Id &videoId);

        VideoMetadata::PreferedStreamContextSource _contextSource;
        VideoMetadata::Id _videoId;
        SignatureDecipherer* _decipherer = nullptr;
        QString _title;
        int _duration = 0;
        int _sts = 0;

        QString _WatchPage_dashManifestUrl;
        QString _WatchPage_raw_playerConfigStreams;
        QJsonArray _WatchPage_raw_playerResponseStreams;
        QDateTime _WatchPage_expireAt;
        QDateTime _WatchPage_requestedAt;

        static promise::Defer _from_VideoInfo(PlayerConfig &pConfig);
        static promise::Defer _from_WatchPage(PlayerConfig &pConfig);

        static promise::Defer _downloadRaw_VideoEmbedPageHtml(const VideoMetadata::Id &videoId);
        static promise::Defer _downloadRaw_WatchPageHtml(const VideoMetadata::Id &videoId);
        
        promise::Defer _downloadAndfillFrom_PlayerSource(const QString &playerSourceUrl);

        promise::Defer _fillFrom_WatchPageHtml(const DownloadedUtf8 &dl);
        promise::Defer _fillFrom_VideoEmbedPageHtml(const DownloadedUtf8 &dl);
        promise::Defer _fillFrom_PlayerSource(const DownloadedUtf8 &dl, const QString &playerSourceUrl);
        
        static QJsonObject _extractPlayerConfigFromRawSource(const DownloadedUtf8 &rawSource, const QRegularExpression &regex);
        
        //extraction helpers
        static QString _playerSourceUrl(const QJsonObject &playerConfig);
        static int _sts(const DownloadedUtf8 &dl);
        
};