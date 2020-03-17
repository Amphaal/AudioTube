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
        static void isStreamAvailable(VideoMetadata* toCheck, bool* checkEnded = nullptr, bool* success = nullptr);

    private:
        static promise::Defer _getVideoEmbedPageRawData(VideoMetadata* metadata);
        static promise::Defer _getVideoInfoDic(VideoMetadata* metadata);

        static VideoMetadata* _augmentMetadataWithPlayerConfiguration(VideoMetadata* metadata, const QByteArray &videoEmbedPageRequestData);
        static VideoMetadata* _augmentMetadataWithVideoInfos(VideoMetadata* metadata, SignatureDecipherer* decipherer, const QByteArray &videoInfoRawResponse, const QDateTime &tsRequest);

        static promise::Defer _downloadVideoInfosAndAugmentMetadata(VideoMetadata* metadata);
        static QString _getApiUrl(const QString &videoId);
        static promise::Defer _getVideoInfosRawData(VideoMetadata* metadata);
        static QList<QString> _extractVideoIdsFromHTTPRequest(const QByteArray &requestData);
        static QList<VideoMetadata*> _videoIdsToMetadataList(const QList<QString> &videoIds);
};