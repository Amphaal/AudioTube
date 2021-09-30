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

#include <spdlog/spdlog.h>

#include "NetworkFetcher.h"

promise::Promise AudioTube::NetworkFetcher::fromPlaylistUrl(const std::string &url) {
    return promise_dl_HTTPS(url)
            .then(&_extractVideoIdsFromHTTPRequest)
            .then(&_videoIdsToMetadataList);
}

promise::Promise AudioTube::NetworkFetcher::refreshMetadata(VideoMetadata* toRefresh, bool force) {
    // check if soft refresh...
    if (!force && !toRefresh->audioStreams()->isExpired()) return promise::resolve(toRefresh);

    // if not, reset failure flag and emit event
    toRefresh->setFailure(false);
    toRefresh->OnMetadataFetching();

    // workflow...
    return promise::newPromise([=](promise::Defer d) {
        // on error default behavior
        auto whenFailed = [=]() {
           toRefresh->setFailure(true);
           toRefresh->setRanOnce();
           d.reject();
        };

        _refreshMetadata(toRefresh)
        .then([=]() {
            // success !
            toRefresh->setRanOnce();
            toRefresh->OnMetadataRefreshed();
            d.resolve(toRefresh);
        })
        .fail([=](const std::runtime_error &exception) {
            spdlog::warn("AudioTube : {}", exception.what());
            whenFailed();
        })
        .fail([=](const std::logic_error &exception) {
            spdlog::warn("AudioTube : {}", exception.what());
            whenFailed();
        })
        .fail([=](const nlohmann::detail::exception &exception) {
            spdlog::warn("AudioTube : {}", exception.what());
            whenFailed();
        })
        .fail([=]() {
            spdlog::warn("AudioTube : unhandled error :'(");
            whenFailed();
        });
    });
}

bool AudioTube::NetworkFetcher::isStreamAvailable(VideoMetadata* toCheck) {
    auto bestUrl = toCheck->audioStreams()->preferedUrl();
    auto response = AudioTube::NetworkHelper::downloadHTTPS(bestUrl, true);  // download HEAD

    // unstack if redirects
    while(!response.redirectUrl.empty()) {
        response = AudioTube::NetworkHelper::downloadHTTPS(response.redirectUrl, true);
    }

    auto isStreamAvailable = response.hasContentLengthHeader && response.statusCode == 200;

    if(isStreamAvailable) spdlog::debug("Stream Available for [{}] : [{}]", toCheck->id(), bestUrl);

    return isStreamAvailable;
}

promise::Promise AudioTube::NetworkFetcher::_refreshMetadata(VideoMetadata* metadata) {
    auto videoInfoPipeline = PlayerConfig::from_EmbedPage(metadata->id())
    .then([=](const PlayerConfig &pConfig) {
        metadata->setPlayerConfig(pConfig);
        return VideoInfos::fillStreamsManifest(
            metadata->id(),
            metadata->playerConfig(),
            metadata->audioStreams()
        );
    });

    // try videoInfoPipeline first, then watchPagePipeline if 1st failed
    return videoInfoPipeline.fail([=](const std::string &softErr){
        spdlog::debug(softErr);

        auto watchPagePipeline = PlayerConfig::from_WatchPage(metadata->id(), metadata->audioStreams())
        .then([=](const PlayerConfig &pConfig) {
            metadata->setPlayerConfig(pConfig);
        });

        return watchPagePipeline;
    });
}

std::vector<std::string> AudioTube::NetworkFetcher::_extractVideoIdsFromHTTPRequest(const DownloadedUtf8 &requestData) {
    // search...
    jp::VecNum matches;
    jp::RegexMatch rm;
    rm.setRegexObject(&Regexes::HTTPRequestYTVideoIdExtractor)
        .setSubject(&requestData)
        .addModifier("gm")
        .setNumberedSubstringVector(&matches)
        .match();

    // check
    if (!matches.size()) throw std::logic_error("[DASH] No YT IDs found in HTTP request");

    // iterate
    std::vector<std::string> idsList;
    for (auto &submatches : matches) {
        auto videoId = submatches[1];
        idsList.push_back(videoId);
    }

    // if no ids
    if (!idsList.size()) throw std::logic_error("no playlist metadata container found !");

    // return
    return idsList;
}

std::vector<AudioTube::VideoMetadata*> AudioTube::NetworkFetcher::_videoIdsToMetadataList(const std::vector<std::string> &videoIds) {
    std::vector<VideoMetadata*> out;
    for (const auto &id : videoIds) {
        out.push_back(VideoMetadata::fromVideoId(id));
    }
    return out;
}
