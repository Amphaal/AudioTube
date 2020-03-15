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

#include "YoutubeAudioStreamInfos.h"

class YoutubeVideoMetadata : public QObject {
    
    Q_OBJECT

    public:
        using Id = QString;
        static YoutubeVideoMetadata* fromVideoUrl(const QString &url);
        static QString urlFromVideoId(const QString &videoId);
        static QRegularExpression getYoutubeUrlMatcher();
        
        YoutubeVideoMetadata(const YoutubeVideoMetadata::Id &videoId);

        YoutubeVideoMetadata::Id id() const;
        QString title() const;
        QString url() const;
        QString sts() const;
        QString playerSourceUrl() const;
        int duration();
        bool isMetadataValid();
        bool hasFailed();

        void setSts(const QString &sts);
        void setPlayerSourceUrl(const QString &pSourceUrl);
        void setTitle(const QString &title);
        void setDuration(int durationInSeconds);
        void setExpirationDate(const QDateTime &expiration);
        void setAudioStreamInfos(const YoutubeAudioStreamInfos &streamInfos);
        void setFailure(bool failed);

        const YoutubeAudioStreamInfos& audioStreams() const;

    signals:
        void metadataFetching();
        void metadataRefreshed();
        void streamFailed();

    private:
        int _durationInSeconds = -1;
        YoutubeVideoMetadata::Id _videoId;
        QString _url;
        QString _title;
        bool _failed = false;

        QString _sts;
        QString _playerSourceUrl;

        YoutubeAudioStreamInfos _audioStreamInfos;

        QDateTime _validUntil;

        QHash<int, QHash<QString, QString>> _sourceUrlsByItag;
        QHash<int, QString> _audioTypeByItag;

        static inline QRegularExpression _ytRegexIdFinder = QRegularExpression("(?:youtube\\.com|youtu.be).*?(?:v=|embed\\/)(?<videoId>[\\w\\-]+)");
};

Q_DECLARE_METATYPE(YoutubeVideoMetadata*)