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

#include "NetworkFetcher.h"

promise::Defer AudioTube::NetworkFetcher::fromPlaylistUrl(const std::string &url) {
    return download(url)
            .then(&_extractVideoIdsFromHTTPRequest)
            .then(&_videoIdsToMetadataList);
}

promise::Defer AudioTube::NetworkFetcher::refreshMetadata(VideoMetadata* toRefresh, bool force) {
    // check if soft refresh...
    if (!force && !toRefresh->audioStreams()->isExpired()) return promise::resolve(toRefresh);

    // if not, reset failure flag and emit event
    toRefresh->setFailure(false);
    emit toRefresh->metadataFetching();

    // workflow...
    return promise::newPromise([=](promise::Defer d){
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
            emit toRefresh->metadataRefreshed();
            d.resolve(toRefresh);
        })
        .fail([=](const std::runtime_error &exception) {
            spdlog::warn("AudioTube : {}", exception.what());
            whenFailed();
        })
        .fail([=](const std::logic_error &exception) {
            spdlog::warn("AudioTube : {}", exception.what());
            whenFailed();
        });
    });
}

void AudioTube::NetworkFetcher::isStreamAvailable(VideoMetadata* toCheck, bool* checkEnded, std::string* urlSuccessfullyRequested) {
    auto bestUrl = toCheck->audioStreams()->preferedUrl();

    download(bestUrl, true)
        .then([=](){
            *urlSuccessfullyRequested = bestUrl.toString();
        })
        .finally([=](){
            *checkEnded = true;
        });
}

promise::Defer AudioTube::NetworkFetcher::_refreshMetadata(VideoMetadata* metadata) {
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

QList<std::string> AudioTube::NetworkFetcher::_extractVideoIdsFromHTTPRequest(const DownloadedUtf8 &requestData) {
    // search...
    auto results = Regexes::HTTPRequestYTVideoIdExtractor.globalMatch(requestData);

    // return list
    QList<std::string> idsList;

    // iterate
    while (results.hasNext()) {
        auto match = results.next();  // next

        // if match
        if (match.hasMatch()) {
            auto found = match.captured("videoId");  // videoId

            if (!idsList.contains(found)) {
                idsList.append(found);
            }
        }
    }

    // if no ids
    if (!idsList.count()) throw std::logic_error("no playlist metadata container found !");

    // return
    return idsList;
}

QList<AudioTube::VideoMetadata*> AudioTube::NetworkFetcher::_videoIdsToMetadataList(const QList<std::string> &videoIds) {
    QList<VideoMetadata*> out;
    for (const auto &id : videoIds) {
        out.append(VideoMetadata::fromVideoId(id));
    }
    return out;
}
