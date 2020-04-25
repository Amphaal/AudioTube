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

#include <QDateTime>

#include <QObject>
#include <QHash>
#include <QString>

#include <QDebug>

#include "PlayerConfig.h"
#include "StreamsManifest.h"

namespace AudioTube {

class VideoMetadata : public QObject {
    Q_OBJECT

 public:
    enum InstantiationType {
        InstFromId,
        InstFromUrl
    };
    VideoMetadata(const QString &IdOrUrl, const InstantiationType &type);

    static VideoMetadata* fromVideoUrl(const QString &url);
    static VideoMetadata* fromVideoId(const QString &videoId);

    PlayerConfig::VideoId id() const;
    QString url() const;
    bool hasFailed() const;
    bool ranOnce() const;

    void setFailure(bool failed);
    void setRanOnce();

    void setPlayerConfig(const PlayerConfig &playerConfig);

    StreamsManifest* audioStreams();
    const PlayerConfig& playerConfig() const;

 signals:
    void metadataFetching();
    void metadataRefreshed();
    void streamFailed();

 private:
    static QString _urlFromVideoId(const QString &videoId);

    PlayerConfig::VideoId _videoId;
    QString _url;
    bool _failed = false;
    bool _ranOnce = false;

    PlayerConfig _playerConfig;
    StreamsManifest _audioStreams;
};

}  // namespace AudioTube

Q_DECLARE_METATYPE(AudioTube::VideoMetadata*)
