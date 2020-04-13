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

#include "VideoInfos.h"

promise::Defer VideoInfos::fillStreamsManifest(const PlayerConfig::VideoId &videoId, const PlayerConfig &playerConfig, StreamsManifest* manifest) {
    
    //set request date
    manifest->setRequestedAt(QDateTime::currentDateTime());
    
    //pipeline
    return _downloadRaw_VideoInfos(videoId, playerConfig.sts())
            .then([=](const DownloadedUtf8 &dl) {
                return _fillFrom_VideoInfos(dl, manifest, playerConfig);
            });
}

promise::Defer VideoInfos::_downloadRaw_VideoInfos(const PlayerConfig::VideoId &videoId, const QString &sts) {
    
    auto apiUrl = QStringLiteral(u"https://youtube.googleapis.com/v/") + videoId;
    auto encodedApiUrl = QString::fromUtf8(QUrl::toPercentEncoding(apiUrl));

    auto requestUrl = QStringLiteral(u"https://www.youtube.com/get_video_info?video_id=%1&el=embedded&eurl=%3&hl=en&sts=%2")
        .arg(videoId).arg(sts).arg(encodedApiUrl);

    return download(requestUrl);

}

promise::Defer VideoInfos::_fillFrom_VideoInfos(const DownloadedUtf8 &dl, StreamsManifest* manifest, const PlayerConfig &playerConfig) {
    return promise::newPromise([=](promise::Defer d) {
 
        //as string then to query
        QUrlQuery videoInfos(dl);

        //get player response
        auto playerResponseAsStr = videoInfos.queryItemValue("player_response", QUrl::ComponentFormattingOption::FullyDecoded);
        auto playerResponse = QJsonDocument::fromJson(playerResponseAsStr.toUtf8());
        if(playerResponseAsStr.isEmpty() || playerResponse.isNull()) {
            throw std::logic_error("Player response is missing !");
        }

        //check if is live
        auto videoDetails = playerResponse[QStringLiteral(u"videoDetails")].toObject();
        auto isLiveStream = videoDetails.value(QStringLiteral(u"isLive")).toBool();
        if(isLiveStream) {
            throw std::logic_error("Live streams are not handled for now!");
        }

        //check playability status
        auto playabilityStatus = playerResponse[QStringLiteral(u"playabilityStatus")].toObject();
        auto pStatus = playabilityStatus.value(QStringLiteral(u"status")).toString();
        if(pStatus.toLower() == "error") {
            throw std::logic_error("This video is not available !");
        }

        //check reason, throw soft error
        auto pReason = playabilityStatus.value(QStringLiteral(u"reason")).toString();
        if(!pReason.isNull()) {
            throw QString(qUtf8Printable(QString("This video is not available though VideoInfo : %1").arg(pReason)));
        }

        //get streamingData
        auto streamingData = playerResponse[QStringLiteral(u"streamingData")].toObject();
        if(streamingData.isEmpty()) {
            throw std::logic_error("An error occured while fetching video infos");
        }

        //find expiration
        auto expiresIn = streamingData.value(QStringLiteral(u"expiresInSeconds")).toString();
        if(expiresIn.isEmpty()) {
            throw std::logic_error("An error occured while fetching video infos");
        }

        //set expiration date
        manifest->setSecondsUntilExpiration((qint64)expiresIn.toDouble());

        //raw stream infos
        auto raw_playerConfigStreams = videoInfos.queryItemValue(QStringLiteral(u"adaptive_fmts"), QUrl::ComponentFormattingOption::FullyDecoded);
        auto raw_playerResponseStreams = streamingData[QStringLiteral(u"adaptiveFormats")].toArray();

        //feed
        manifest->feedRaw_PlayerConfig(raw_playerConfigStreams, playerConfig.decipherer());
        manifest->feedRaw_PlayerResponse(raw_playerResponseStreams, playerConfig.decipherer());
        
        //DASH manifest handling
        auto dashManifestUrl = streamingData.value(QStringLiteral(u"dashManifestUrl")).toString();
        d.resolve(dashManifestUrl);
        
    }).then([=](const QString &dashManifestUrl){
        auto mayFetchRawDASH = dashManifestUrl.isEmpty() ? promise::resolve() : download(dashManifestUrl).then([=](const DownloadedUtf8 &dl) {
            manifest->feedRaw_DASH(dl, playerConfig.decipherer());
        });
        return mayFetchRawDASH;
    });
}