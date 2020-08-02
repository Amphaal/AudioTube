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

#include "PlayerConfig.h"

AudioTube::PlayerConfig::PlayerConfig() {}

AudioTube::PlayerConfig::PlayerConfig(const PlayerConfig::ContextSource &streamContextSource, const PlayerConfig::VideoId &videoId) {
    this->_contextSource = streamContextSource;
}

std::string AudioTube::PlayerConfig::sts() const {
    return this->_sts;
}

int AudioTube::PlayerConfig::duration() const {
    return this->_duration;
}

std::string AudioTube::PlayerConfig::title() const {
    return this->_title;
}

const AudioTube::SignatureDecipherer* AudioTube::PlayerConfig::decipherer() const {
    return this->_decipherer;
}

AudioTube::PlayerConfig::ContextSource AudioTube::PlayerConfig::contextSource() const {
    return this->_contextSource;
}

void AudioTube::PlayerConfig::fillFromVideoInfosDetails(const std::string &title, int duration) {
    this->_title = title;
    this->_duration = duration;
}

promise::Defer AudioTube::PlayerConfig::from_EmbedPage(const PlayerConfig::VideoId &videoId) {
    // pipeline
    return _downloadRaw_VideoEmbedPageHtml(videoId)
            .then([=](const DownloadedUtf8 &dl) {
                PlayerConfig pConfig(PlayerConfig::ContextSource::EmbedPage, videoId);
                return pConfig._fillFrom_VideoEmbedPageHtml(dl);
            });
}

promise::Defer AudioTube::PlayerConfig::from_WatchPage(const PlayerConfig::VideoId &videoId, StreamsManifest* streamsManifest) {
    // define requested at timestamp
    streamsManifest->setRequestedAt(std::time_t::currentDateTime());

    // pipeline
    return _downloadRaw_WatchPageHtml(videoId)
            .then([=](const DownloadedUtf8 &dl) {
                PlayerConfig pConfig(PlayerConfig::ContextSource::WatchPage, videoId);
                return pConfig._fillFrom_WatchPageHtml(dl, streamsManifest);
            });
}

promise::Defer AudioTube::PlayerConfig::_downloadRaw_VideoEmbedPageHtml(const PlayerConfig::VideoId &videoId) {
    auto url = std::string("https://www.youtube.com/embed/" + videoId + "?hl=en");
    return downloadHTTPS(url);
}

promise::Defer AudioTube::PlayerConfig::_downloadRaw_WatchPageHtml(const PlayerConfig::VideoId &videoId) {
    auto url = std::string("https://www.youtube.com/watch?v=" + videoId + "&bpctr=9999999999&hl=en");
    return downloadHTTPS(url);
}


QJsonObject AudioTube::PlayerConfig::_extractPlayerConfigFromRawSource(const DownloadedUtf8 &rawSource, const std::regex &regex) {
    auto playerConfigAsStr = regex.match(rawSource).captured("playerConfig");
    auto playerConfig = QJsonDocument::fromJson(playerConfigAsStr.toUtf8()).object();

    // check config exists
    if (playerConfigAsStr.isEmpty() || playerConfig.isEmpty()) {
        throw std::logic_error("Player response is missing !");
    }

    return playerConfig;
}

std::string AudioTube::PlayerConfig::_playerSourceUrl(const QJsonObject &playerConfig) {
    auto playerSourceUrlPath = playerConfig[std::string(u"assets")].toObject()[std::string(u"js")].toString();
    if (playerSourceUrlPath.isEmpty()) throw std::logic_error("Player source URL is cannot be found !");
    return std::string("https://www.youtube.com") + playerSourceUrlPath;
}

promise::Defer AudioTube::PlayerConfig::_downloadAndfillFrom_PlayerSource(const std::string &playerSourceUrl) {
    return promise::newPromise([=](promise::Defer d){
        // if cached is found, return it
        auto cachedDecipherer = SignatureDecipherer::fromCache(playerSourceUrl);
        if (cachedDecipherer) {
            this->_decipherer = cachedDecipherer;
        }

        auto stsNeeded = this->_contextSource == PlayerConfig::ContextSource::EmbedPage;

        d.resolve((bool)cachedDecipherer, stsNeeded);
    })
    .then([=](bool cachedDecipherer, bool stsNeeded) {
        // if no cached decipherer or if STS is needed
        if (!cachedDecipherer || stsNeeded) {
            return downloadHTTPS(playerSourceUrl)
            .then([=](const DownloadedUtf8 &dl) {
                // generate decipherer
                if (!cachedDecipherer) {
                    this->_decipherer = SignatureDecipherer::create(
                        playerSourceUrl,
                        dl
                    );
                }

                // fetch STS
                if (stsNeeded) {
                    this->_sts = _getSts(dl);
                }
            });
        }

        return promise::resolve();
    });
}

promise::Defer AudioTube::PlayerConfig::_fillFrom_VideoEmbedPageHtml(const DownloadedUtf8 &dl) {
    return promise::newPromise([=](promise::Defer d) {
        auto playerConfig = _extractPlayerConfigFromRawSource(dl, Regexes::PlayerConfigExtractorFromEmbed);

        // player source
        auto playerSourceUrl = _playerSourceUrl(playerConfig);
        d.resolve(playerSourceUrl);
    })
    .then([=](const std::string &playerSourceUrl){
        return this->_downloadAndfillFrom_PlayerSource(playerSourceUrl);
    })
    .then([=]() {
        return *this;
    });
}

promise::Defer AudioTube::PlayerConfig::_fillFrom_WatchPageHtml(const DownloadedUtf8 &dl, StreamsManifest* streamsManifest) {
    return promise::newPromise([=](promise::Defer d) {
        // get player config JSON
        auto playerConfig = _extractPlayerConfigFromRawSource(dl, Regexes::PlayerConfigExtractorFromWatchPage);

        // Get player response JSON
        auto args = playerConfig[std::string(u"args")].toObject();
        auto playerResponseAsStr = args[std::string(u"player_response")].toString();
        auto playerResponse = QJsonDocument::fromJson(playerResponseAsStr.toUtf8());
        if (playerResponseAsStr.isEmpty() || playerResponse.isNull()) {
            throw std::logic_error("Player response is missing !");
        }

        // fetch and check video infos
        auto videoDetails = playerResponse[std::string(u"videoDetails")].toObject();
        this->_title = videoDetails[std::string(u"title")].toString();
        this->_duration = videoDetails[std::string(u"lengthSeconds")].toString().toInt();
        auto isLive = videoDetails[std::string(u"isLive")].toBool();

        if (isLive) throw std::logic_error("Live streams are not handled for now!");
        if (this->_title.isEmpty()) throw std::logic_error("Video title cannot be found !");
        if (!this->_duration) throw std::logic_error("Video length cannot be found !");

        // check playability status, throw err
        auto playabilityStatus = playerResponse[std::string(u"playabilityStatus")].toObject();
        auto pReason = playabilityStatus.value(std::string(u"reason")).toString();
        if (!pReason.isNull()) {
            throw std::logic_error("This video is not available though WatchPage : %1");
        }

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
        streamsManifest->setSecondsUntilExpiration((qint64)expiresIn.toDouble());

        // raw stream infos
        auto raw_playerConfigStreams = args.value(std::string(u"adaptive_fmts")).toString();
        auto raw_playerResponseStreams = streamingData[std::string(u"adaptiveFormats")].toArray();

        // feed
        streamsManifest->feedRaw_PlayerConfig(raw_playerConfigStreams, this->_decipherer);
        streamsManifest->feedRaw_PlayerResponse(raw_playerResponseStreams, this->_decipherer);

        // DASH manifest handling
        auto dashManifestUrl = streamingData.value(std::string(u"dashManifestUrl")).toString();

        // Extract player source URL
        auto playerSourceUrl = _playerSourceUrl(playerConfig);

        d.resolve(playerSourceUrl, dashManifestUrl);
    })
    .then([=](const std::string &playerSourceUrl, const std::string &dashManifestUrl){
        return this->_downloadAndfillFrom_PlayerSource(playerSourceUrl)
        .then([=](){
            return dashManifestUrl;
        });
    })
    .then([=](const std::string &dashManifestUrl){
       auto mayFetchRawDASH = dashManifestUrl.isEmpty() ? promise::resolve() : downloadHTTPS(dashManifestUrl).then([=](const DownloadedUtf8 &dl) {
            streamsManifest->feedRaw_DASH(dl, this->_decipherer);
        });
        return mayFetchRawDASH;
    })
    .then([=]() {
        return *this;
    });
}

std::string AudioTube::PlayerConfig::_getSts(const DownloadedUtf8 &dl) {
    std::string sts;

    auto match = Regexes::STSFinder.match(dl);
    if (match.hasMatch()) {
        sts = match.captured("sts");  // sts
    } else {
        throw std::logic_error("STS value cannot be found !");
    }

    return sts;
}
