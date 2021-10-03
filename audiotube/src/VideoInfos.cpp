// AudioTube C++
// C++ fork based on https://github.com/Tyrrrz/YoutubeExplode
// Copyright (C) 2019-2021 Guillaume Vara

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "VideoInfos.h"

promise::Promise AudioTube::VideoInfos::fillStreamsManifest(const PlayerConfig::VideoId &videoId, PlayerConfig* playerConfig, StreamsManifest* manifest) {
    // set request date
    auto now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );
    manifest->setRequestedAt(now);

    // pipeline
    return _downloadRaw_VideoInfos(videoId, playerConfig->sts())
            .then([=](const DownloadedUtf8 &dl) {
                return _fillFrom_VideoInfos(dl, manifest, playerConfig);
            });
}

promise::Promise AudioTube::VideoInfos::_downloadRaw_VideoInfos(const PlayerConfig::VideoId &videoId, const std::string &sts) {
    auto apiUrl = std::string("https://youtube.googleapis.com/v/") + videoId;
    auto encodedApiUrl = Url::encode(apiUrl);

    auto requestUrl = std::string("https://www.youtube.com/get_video_info?video_id=") + videoId + "&el=embedded&eurl=" + encodedApiUrl + "&hl=en&sts=" + sts;

    return promise_dl_HTTPS(requestUrl);
}

promise::Promise AudioTube::VideoInfos::_fillFrom_VideoInfos(const DownloadedUtf8 &dl, StreamsManifest* manifest, PlayerConfig *playerConfig) {
    return promise::newPromise([=](promise::Defer d) {
        // as string then to query
        UrlQuery videoInfos(dl);

        // get player response
        auto playerResponseAsStr = videoInfos["player_response"].percentDecoded();
        auto playerResponse = nlohmann::json::parse(playerResponseAsStr);
        if (playerResponseAsStr.empty() || playerResponse.is_null()) {
            throw std::logic_error("Player response is missing !");
        }

        auto videoDetails = playerResponse["videoDetails"];

        // check playability status
        auto playabilityStatus = playerResponse["playabilityStatus"];

        // check reason, throw soft error
        auto pReason = playabilityStatus["reason"];
        if (!pReason.is_null()) {
            throw std::string("This video is not available though VideoInfo : ") + pReason.get<std::string>();
        }

        auto pStatus = playabilityStatus["status"].get<std::string>();
        if (pStatus != "OK") {
            throw std::logic_error("This video is not available !");
        }

        // check if is live
        auto isLiveStream = videoDetails["isLiveContent"].get<bool>();
        if (isLiveStream) {
            throw std::logic_error("Live streams are not handled for now!");
        }

        // get title and duration
        auto title = videoDetails["title"].get<std::string>();
        std::replace(title.begin(), title.end(), '+', ' ');
        auto duration = safe_stoi(videoDetails["lengthSeconds"].get<std::string>());

        if (title.empty()) throw std::logic_error("Video title cannot be found !");
        if (duration < 0) throw std::logic_error("Video length cannot be found !");

        playerConfig->fillFromVideoInfosDetails(title, duration);

        // get streamingData
        auto streamingData = playerResponse["streamingData"];
        if (streamingData.is_null()) {
            throw std::logic_error("An error occured while fetching video infos");
        }

        // find expiration
        auto expiresIn = streamingData["expiresInSeconds"].get<std::string>();
        if (expiresIn.empty()) {
            throw std::logic_error("An error occured while fetching video infos");
        }

        // set expiration date
        auto ei_cast = safe_stoi(expiresIn);
        if (ei_cast < 0) {
            throw std::logic_error("An error occured while fetching video infos");
        }
        manifest->setSecondsUntilExpiration((unsigned int)ei_cast);

        // raw stream infos
        auto raw_playerConfigStreams = videoInfos["adaptive_fmts"].percentDecoded();
        auto raw_playerResponseStreams = streamingData["adaptiveFormats"];

        // feed
        manifest->feedRaw_PlayerConfig(raw_playerConfigStreams, playerConfig->decipherer());
        manifest->feedRaw_PlayerResponse(raw_playerResponseStreams, playerConfig->decipherer());

        // DASH manifest handling
        std::string dashManifestUrl;
        auto dmu = streamingData["dashManifestUrl"];
        if(!dmu.is_null()) dashManifestUrl = dmu.get<std::string>();

        d.resolve(dashManifestUrl);
    })
    .then([=](const std::string &dashManifestUrl){
        auto mayFetchRawDASH = dashManifestUrl.empty() ? promise::resolve() : promise_dl_HTTPS(dashManifestUrl).then([=](const DownloadedUtf8 &dl) {
            manifest->feedRaw_DASH(dl, playerConfig->decipherer());
        });
        return mayFetchRawDASH;
    });
}
