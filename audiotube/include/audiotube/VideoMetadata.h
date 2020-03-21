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

#include "PlayerConfiguration.h"

class VideoMetadata : public QObject {
    
    Q_OBJECT

    public:
        enum PreferedPlayerConfigFetchingMethod {
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
        PreferedPlayerConfigFetchingMethod preferedPlayerConfigFetchingMethod() const;
        PlayerConfiguration::PreferedAudioStreamsInfosSource preferedAudioStreamsInfosSource() const;

        void setTitle(const QString &title);
        void setDuration(int durationInSeconds);
        void setExpirationDate(const QDateTime &expiration);
        void setAudioStreamsPackage(const PlayerConfiguration::AudioStreamsPackage &streamInfos);
        void setFailure(bool failed);
        void setRanOnce();
        void setPreferedPlayerConfigFetchingMethod(const PreferedPlayerConfigFetchingMethod &method);

        const PlayerConfiguration::AudioStreamUrlByITag& audioStreams() const;

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

        PlayerConfiguration::AudioStreamUrlByITag _audioStreamInfos;

        PreferedPlayerConfigFetchingMethod _preferedPlayerConfigFetchingMethod = PreferedPlayerConfigFetchingMethod::Unknown;
        PlayerConfiguration::PreferedAudioStreamsInfosSource _preferedAudioStreamsInfosSource = PlayerConfiguration::PreferedAudioStreamsInfosSource::Unknown;

        QDateTime _validUntil;

        QHash<int, QHash<QString, QString>> _sourceUrlsByItag;
        QHash<int, QString> _audioTypeByItag;

        static inline QRegularExpression _ytRegexIdFinder = QRegularExpression("(?:youtube\\.com|youtu.be).*?(?:v=|embed\\/)(?<videoId>[\\w\\-]+)");
};

Q_DECLARE_METATYPE(VideoMetadata*)