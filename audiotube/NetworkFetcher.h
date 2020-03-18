////////////////////////////////////////////////////////////////////////////
// BASED ON THE WORK OF Alexey "Tyrrrz" Golub (https://github.com/Tyrrrz) //
////////////////////////////////////////////////////////////////////////////

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

#include "_base/_NetworkHelper.h"
#include "_base/_DebugHelper.h"

#include "PlayerConfiguration.hpp"
#include "VideoMetadata.h"

class NetworkFetcher : public NetworkHelper {
    
    public:
        static promise::Defer fromPlaylistUrl(const QString &url);
        static promise::Defer refreshMetadata(VideoMetadata* toRefresh, bool force = false);
        static void isStreamAvailable(VideoMetadata* toCheck, bool* checkEnded = nullptr, QString* urlSuccessfullyRequested = nullptr);

    private:
        static promise::Defer _getVideoEmbedPageHtml(const VideoMetadata::Id &videoId);
        static promise::Defer _getWatchPageHtml(const VideoMetadata::Id &videoId);
        static promise::Defer _getVideoInfosDic(const VideoMetadata::Id &videoId);

        static promise::Defer _getPlayerConfiguration(VideoMetadata* metadata);
        static promise::Defer _getPlayerConfiguration_VideoInfo(VideoMetadata* metadata);
        static promise::Defer _getPlayerConfiguration_WatchPage(VideoMetadata* metadata);

        static promise::Defer _extractDataFrom_VideoInfos(const DownloadedUtf8 &dl, const QDateTime &requestedAt);
        static promise::Defer _extractDataFrom_EmbedPageHtml(const DownloadedUtf8 &videoEmbedPageRequestData);
        static promise::Defer _extractDataFrom_WatchPage(const DownloadedUtf8 &dl, const QDateTime &requestedAt);

        static QString _extractPlayerSourceUrlFromPlayerConfig(const QJsonObject &playerConfig);
        static QJsonObject _extractPlayerConfigFromRawSource(const DownloadedUtf8 &rawSource, const QRegularExpression &regex);

        static promise::Defer _fetchDecipherer(const PlayerConfiguration &playerConfig);

        static QList<QString> _extractVideoIdsFromHTTPRequest(const DownloadedUtf8 &requestData);
        static QList<VideoMetadata*> _videoIdsToMetadataList(const QList<QString> &videoIds);
};