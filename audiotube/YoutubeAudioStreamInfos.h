////////////////////////////////////////////////////////////////////////////
// BASED ON THE WORK OF Alexey "Tyrrrz" Golub (https://github.com/Tyrrrz) //
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QString>

#include <QHash>
#include <QString>
#include <QUrlQuery>

#include <QJsonObject>
#include <QJsonArray>

#include "YoutubeSignatureDecipherer.h"

#include <QDebug>

class YoutubeAudioStreamInfos {

    public:
        using InfosByAudioMime = QHash<QString, QHash<QString, QString>>;
        using RawInfosByAudioMime = QList<QHash<QString, QString>>;
        struct UrlMimePair {
            QString mime;
            QString decodedUrl;
        };

        YoutubeAudioStreamInfos();
        YoutubeAudioStreamInfos(YoutubeSignatureDecipherer* decipherer, const QString &urlQueryAsRawStr);
        YoutubeAudioStreamInfos(const QJsonArray &adaptativeFormatsWithUnsignedUrls);

        const QString streamUrl(const QString &mime) const;
        const QList<QString> availableAudioMimes() const;
        const UrlMimePair getPreferedMimeSourcePair() const;
    
    private:
        InfosByAudioMime _InfosByAudioMime;

        static RawInfosByAudioMime _generatRawAdaptiveStreamInfosFromUrlQuery(const QString &urlQueryAsRawStr);
        static RawInfosByAudioMime _generatRawAdaptiveStreamInfosFromJSON(const QJsonArray &jsonArray);

        void _initFromUrlQuery(YoutubeSignatureDecipherer* decipherer, const RawInfosByAudioMime &rawData);
};