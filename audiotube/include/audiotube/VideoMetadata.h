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
#include <QRegularExpression>

#include <QDebug>

#include "VideoContext.h"

class VideoMetadata : public QObject {
    
    Q_OBJECT

    public:
        enum PreferedStreamContextSource {
            Unknown,
            VideoInfo,
            WatchPage
        };
        enum InstantiationType {
            InstFromId,
            InstFromUrl
        };  

        using Id = QString;
        VideoMetadata(const QString &IdOrUrl, const InstantiationType &type);

        static VideoMetadata* fromVideoUrl(const QString &url);
        static VideoMetadata* fromVideoId(const QString &videoId);
        static QRegularExpression getUrlMatcher();
        
        QUrl getBestAvailableStreamUrl() const;
        VideoMetadata::Id id() const;
        QString title() const;
        QString url() const;
        int duration() const;
        bool isMetadataValid() const;
        bool hasFailed() const;
        bool ranOnce() const;
        PreferedStreamContextSource preferedStreamContextSource() const;
        VideoContext::PreferedAudioStreamsInfosSource preferedAudioStreamsInfosSource() const;

        void setTitle(const QString &title);
        void setDuration(int durationInSeconds);
        void setExpirationDate(const QDateTime &expiration);
        void setAudioStreamsPackage(const VideoContext::AudioStreamsPackage &streamInfos);
        void setFailure(bool failed);
        void setRanOnce();
        void setPreferedStreamContextSource(const PreferedStreamContextSource &method);

        const VideoContext::AudioStreamUrlByITag& audioStreams() const;

    signals:
        void metadataFetching();
        void metadataRefreshed();
        void streamFailed();

    private:    
        static QString _urlFromVideoId(const QString &videoId);

        int _durationInSeconds = -1;
        VideoMetadata::Id _videoId;
        QString _url;
        QString _title;
        bool _failed = false;
        bool _ranOnce = false;

        VideoContext::AudioStreamUrlByITag _audioStreamInfos;

        PreferedStreamContextSource _preferedStreamContextSource = PreferedStreamContextSource::Unknown;
        VideoContext::PreferedAudioStreamsInfosSource _preferedAudioStreamsInfosSource = VideoContext::PreferedAudioStreamsInfosSource::Unknown;

        QDateTime _validUntil;

        QHash<int, QHash<QString, QString>> _sourceUrlsByItag;
        QHash<int, QString> _audioTypeByItag;

        static inline QRegularExpression _ytRegexIdFinder = QRegularExpression("(?:youtube\\.com|youtu.be).*?(?:v=|embed\\/)(?<videoId>[\\w\\-]+)");
};

Q_DECLARE_METATYPE(VideoMetadata*)