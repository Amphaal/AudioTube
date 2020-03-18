////////////////////////////////////////////////////////////////////////////
// BASED ON THE WORK OF Alexey "Tyrrrz" Golub (https://github.com/Tyrrrz) //
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QDateTime>

#include <QObject>
#include <QHash>
#include <QString>
#include <QRegularExpression>

#include <QDebug>

#include "PlayerConfiguration.hpp"

class VideoMetadata : public QObject {
    
    Q_OBJECT

    public:
        using Id = QString;
        static VideoMetadata fromVideoUrl(const QString &url);
        static VideoMetadata fromVideoId(const QString &videoId);
        static QString urlFromVideoId(const QString &videoId);
        static QRegularExpression getUrlMatcher();
        
        VideoMetadata(const VideoMetadata::Id &videoId);

        VideoMetadata::Id id() const;
        QString title() const;
        QString url() const;
        QString playerSourceUrl() const;
        int duration() const;
        bool isMetadataValid() const;
        bool hasFailed() const;
        bool ranOnce() const;

        void setPlayerSourceUrl(const QString &pSourceUrl);
        void setTitle(const QString &title);
        void setDuration(int durationInSeconds);
        void setExpirationDate(const QDateTime &expiration);
        void setAudioStreamInfos(const PlayerConfiguration::AudioStreamUrlByITag &streamInfos);
        void setFailure(bool failed);
        void setRanOnce();

        const PlayerConfiguration::AudioStreamUrlByITag& audioStreams() const;

    signals:
        void metadataFetching();
        void metadataRefreshed();
        void streamFailed();

    private:
        int _durationInSeconds = -1;
        VideoMetadata::Id _videoId;
        QString _playerSourceUrl;
        QString _url;
        QString _title;
        bool _failed = false;
        bool _ranOnce = false;

        PlayerConfiguration::AudioStreamUrlByITag _audioStreamInfos;

        QDateTime _validUntil;

        QHash<int, QHash<QString, QString>> _sourceUrlsByItag;
        QHash<int, QString> _audioTypeByItag;

        static inline QRegularExpression _ytRegexIdFinder = QRegularExpression("(?:youtube\\.com|youtu.be).*?(?:v=|embed\\/)(?<videoId>[\\w\\-]+)");
};

Q_DECLARE_METATYPE(VideoMetadata*)