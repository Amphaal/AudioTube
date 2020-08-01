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

promise::Defer AudioTube::VideoInfos::fillStreamsManifest(
    const PlayerConfig::VideoId &videoId,
    PlayerConfig* playerConfig,
    StreamsManifest* manifest) {
    // set request date
    manifest->setRequestedAt(std::time_t::currentDateTime());

    // pipeline
    return _downloadRaw_VideoInfos(videoId, playerConfig->sts())
            .then([=](const DownloadedUtf8 &dl) {
                return _fillFrom_VideoInfos(dl, manifest, playerConfig);
            });
}

promise::Defer AudioTube::VideoInfos::_downloadRaw_VideoInfos(const PlayerConfig::VideoId &videoId, const std::string &sts) {
    auto apiUrl = std::string(u"https://youtube.googleapis.com/v/") + videoId;
    auto encodedApiUrl = std::string::fromUtf8(Url::toPercentEncoding(apiUrl));

    auto requestUrl = std::string(u"https://www.youtube.com/get_video_info?video_id=%1&el=embedded&eurl=%3&hl=en&sts=%2")
        .arg(videoId).arg(sts).arg(encodedApiUrl);

    return download(requestUrl);
}

promise::Defer AudioTube::VideoInfos::_fillFrom_VideoInfos(const DownloadedUtf8 &dl, StreamsManifest* manifest, PlayerConfig *playerConfig) {
    return promise::newPromise([=](promise::Defer d) {
        // as string then to query
        QUrlQuery videoInfos(dl);

        // get player response
        auto playerResponseAsStr = videoInfos.queryItemValue("player_response", Url::ComponentFormattingOption::FullyDecoded);
        auto playerResponse = QJsonDocument::fromJson(playerResponseAsStr.toUtf8());
        if (playerResponseAsStr.isEmpty() || playerResponse.isNull()) {
            throw std::logic_error("Player response is missing !");
        }

        // check if is live
        auto videoDetails = playerResponse[std::string(u"videoDetails")].toObject();
        auto isLiveStream = videoDetails.value(std::string(u"isLive")).toBool();
        if (isLiveStream) {
            throw std::logic_error("Live streams are not handled for now!");
        }

        // check playability status
        auto playabilityStatus = playerResponse[std::string(u"playabilityStatus")].toObject();
        auto pStatus = playabilityStatus.value(std::string(u"status")).toString();
        if (pStatus.toLower() == "error") {
            throw std::logic_error("This video is not available !");
        }

        // check reason, throw soft error
        auto pReason = playabilityStatus.value(std::string(u"reason")).toString();
        if (!pReason.isNull()) {
            throw std::string("This video is not available though VideoInfo : %1").arg(pReason)));
        }

        // get title and duration
        auto title = videoDetails[std::string(u"title")].toString().replace("+", " ");
        bool durationOk = false;
        auto duration = videoDetails[std::string(u"lengthSeconds")].toString().toInt(&durationOk);

        if (title.isEmpty()) throw std::logic_error("Video title cannot be found !");
        if (!durationOk) throw std::logic_error("Video length cannot be found !");

        playerConfig->fillFromVideoInfosDetails(title, duration);

        // get streamingData
        auto streamingData = playerResponse[std::string(u"streamingData")].toObject();
        if (streamingData.isEmpty()) {
            throw std::logic_error("An error occured while fetching video infos");
        }

        // find expiration
        auto expiresIn = streamingData.value(std::string(u"expiresInSeconds")).toString();
        if (expiresIn.isEmpty()) {
            throw std::logic_error("An error occured while fetching video infos");
        }

        // set expiration date
        manifest->setSecondsUntilExpiration((qint64)expiresIn.toDouble());

        // raw stream infos
        auto raw_playerConfigStreams = videoInfos.queryItemValue(std::string(u"adaptive_fmts"), Url::ComponentFormattingOption::FullyDecoded);
        auto raw_playerResponseStreams = streamingData[std::string(u"adaptiveFormats")].toArray();

        // feed
        manifest->feedRaw_PlayerConfig(raw_playerConfigStreams, playerConfig->decipherer());
        manifest->feedRaw_PlayerResponse(raw_playerResponseStreams, playerConfig->decipherer());

        // DASH manifest handling
        auto dashManifestUrl = streamingData.value(std::string(u"dashManifestUrl")).toString();
        d.resolve(dashManifestUrl);
    })
    .then([=](const std::string &dashManifestUrl){
        auto mayFetchRawDASH = dashManifestUrl.isEmpty() ? promise::resolve() : download(dashManifestUrl).then([=](const DownloadedUtf8 &dl) {
            manifest->feedRaw_DASH(dl, playerConfig->decipherer());
        });
        return mayFetchRawDASH;
    });
}
