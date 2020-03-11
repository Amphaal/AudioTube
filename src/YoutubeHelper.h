////////////////////////////////////////////////////////////////////////////
// BASED ON THE WORK OF Alexey "Tyrrrz" Golub (https://github.com/Tyrrrz) //
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QList>

#include <QDebug>

#include <QRegularExpression>
#include <QUrlQuery>
#include <QString>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "_base/_NetworkHelper.h"

#include "YoutubeVideoMetadata.h"

class YoutubeHelper : public NetworkHelper {
    
    public:
        static promise::Defer fromPlaylistUrl(const QString &url);
        static promise::Defer refreshMetadata(YoutubeVideoMetadata* toRefresh, bool force = false);

    private:
        static promise::Defer _getVideoEmbedPageRawData(YoutubeVideoMetadata* metadata);
        static YoutubeVideoMetadata* _augmentMetadataWithPlayerConfiguration(YoutubeVideoMetadata* metadata, const QByteArray &videoEmbedPageRequestData);
        static YoutubeVideoMetadata* _augmentMetadataWithVideoInfos(YoutubeVideoMetadata* metadata, YoutubeSignatureDecipherer* decipherer, const QByteArray &videoInfoRawResponse, const QDateTime &tsRequest);

        static promise::Defer _downloadVideoInfosAndAugmentMetadata(YoutubeVideoMetadata* metadata);
        static QString _getApiUrl(const QString &videoId);
        static promise::Defer _getVideoInfosRawData(YoutubeVideoMetadata* metadata);
        static QList<QString> _extractVideoIdsFromHTTPRequest(const QByteArray &requestData);
        static QList<YoutubeVideoMetadata*> _videoIdsToMetadataList(const QList<QString> &videoIds);

        static void _dumpAsJSON(const QUrlQuery &query);
        static void _dumpAsJSON(const QJsonObject &obj);
        static void _dumpAsJSON(const QJsonArray &arr);
        static void _dumpAsJSON(const QJsonDocument &doc);
};