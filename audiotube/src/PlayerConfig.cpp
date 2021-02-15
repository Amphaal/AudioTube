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
    spdlog::debug("PlayerConfig : Trying from [Embed]...");
    // pipeline
    return _downloadRaw_VideoEmbedPageHtml(videoId)
            .then([=](const DownloadedUtf8 &dl) {
                PlayerConfig pConfig(PlayerConfig::ContextSource::EmbedPage, videoId);
                return pConfig._fillFrom_VideoEmbedPageHtml(dl);
            });
}

promise::Defer AudioTube::PlayerConfig::from_WatchPage(const PlayerConfig::VideoId &videoId, StreamsManifest* streamsManifest) {
    spdlog::debug("PlayerConfig : Trying from [WatchPage]...");
    // define requested at timestamp
    auto now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );
    streamsManifest->setRequestedAt(now);

    // pipeline
    return _downloadRaw_WatchPageHtml(videoId)
            .then([=](const DownloadedUtf8 &dl) {
                PlayerConfig pConfig(PlayerConfig::ContextSource::WatchPage, videoId);
                return pConfig._fillFrom_WatchPageHtml(dl, streamsManifest);
            });
}

promise::Defer AudioTube::PlayerConfig::_downloadRaw_VideoEmbedPageHtml(const PlayerConfig::VideoId &videoId) {
    auto url = std::string("https://www.youtube.com/embed/" + videoId + "?hl=en");
    return promise_dl_HTTPS(url);
}

promise::Defer AudioTube::PlayerConfig::_downloadRaw_WatchPageHtml(const PlayerConfig::VideoId &videoId) {
    auto url = std::string("https://www.youtube.com/watch?v=" + videoId + "&bpctr=9999999999&hl=en");
    return promise_dl_HTTPS(url);
}


nlohmann::json AudioTube::PlayerConfig::_extractPlayerConfigFromRawSource(const DownloadedUtf8 &rawSource, const jp::Regex &regexToJSONStart) {
    // search
    jp::VecNum matches;
    jp::RegexMatch rm;
    rm.setRegexObject(&regexToJSONStart)
        .setSubject(&rawSource)
        .addModifier("gm")
        .setNumberedSubstringVector(&matches)
        .match();

    // check if first part is there
    if (matches.size() != 1)
        throw std::logic_error("Failed to extract Player Configuration from raw source");
    auto firstPart = matches[0][1];

    // get the first result of the rest
    rm.setRegexObject(&Regexes::BalancedBraces)
        .setSubject(&firstPart)
        .match();

    // get player config as string
    auto &playerConfigAsStr = matches[0][0];
    if (playerConfigAsStr.empty()) {
        throw std::logic_error("Player response is missing !");
    }

    // try to parse
    auto playerConfig = nlohmann::json::parse(playerConfigAsStr);
    if (playerConfig.is_null()) {
        throw std::logic_error("Cannot parse to JSON the Player Configuration !");
    }

    return playerConfig;
}

std::string AudioTube::PlayerConfig::_playerSourceUrl(const nlohmann::json &playerConfig) {
    auto playerSourceUrlPath = playerConfig["assets"]["js"].get<std::string>();
    if (playerSourceUrlPath.empty()) throw std::logic_error("Player source URL is cannot be found !");
    return std::string("https://www.youtube.com") + playerSourceUrlPath;
}

std::string AudioTube::PlayerConfig::_extractPlayerSourceURLFromRawSource(const DownloadedUtf8 &rawSource) {
    // search for <script> tags
    jp::VecNum matches;
    
    jp::RegexMatch rm;
    rm.setRegexObject(&Regexes::FindScriptSrc)
        .setSubject(&rawSource)
        .addModifier("gm")
        .setNumberedSubstringVector(&matches)
        .match();

    for(auto &src : matches) {
        // check if contains keyword
        auto &grpMatch = src[1];
        if(grpMatch.find("player_ias") == std::string::npos) continue;

        spdlog::debug(grpMatch);

        // returns
        return std::string("https://www.youtube.com") + grpMatch;
    }

    // if not found in regex
    throw std::logic_error("Failed to extract PlayerSourceURL from raw source");
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
            return promise_dl_HTTPS(playerSourceUrl)
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
    std::string playerSourceURL;
    return promise::newPromise([&playerSourceURL, dl](promise::Defer d) {
        playerSourceURL = _extractPlayerSourceURLFromRawSource(dl);
    })
    .then(this->_downloadAndfillFrom_PlayerSource(playerSourceURL))
    .then([=]() {
        return *this;
    });
}

promise::Defer AudioTube::PlayerConfig::_fillFrom_WatchPageHtml(const DownloadedUtf8 &dl, StreamsManifest* streamsManifest) {
    return promise::newPromise([=](promise::Defer d) {
        // get player config JSON
        auto playerConfig = _extractPlayerConfigFromRawSource(dl, Regexes::PlayerConfigExtractorFromWatchPage_JSONStart);

        // fetch and check video infos
        auto videoDetails = playerConfig["videoDetails"];
            this->_title = videoDetails["title"].get<std::string>();
            this->_duration = safe_stoi(videoDetails["lengthSeconds"].get<std::string>());
            auto isLive = videoDetails["isLiveContent"].get<bool>();

        if (isLive) throw std::logic_error("Live streams are not handled for now!");
        if (this->_title.empty()) throw std::logic_error("Video title cannot be found !");
        if (!this->_duration) throw std::logic_error("Video length cannot be found !");

        // check playability status, throw err
        auto playabilityStatus = playerConfig["playabilityStatus"];
        auto pReason = playabilityStatus["reason"];
        if (!pReason.is_null()) {
            throw std::logic_error("This video is not available though WatchPage : " + pReason.get<std::string>());
        }

        // get streamingData
        auto streamingData = playerConfig["streamingData"];
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
        streamsManifest->setSecondsUntilExpiration((unsigned int)ei_cast);

        // raw stream infos
        auto raw_playerResponseStreams = streamingData["adaptiveFormats"];
        streamsManifest->feedRaw_PlayerResponse(raw_playerResponseStreams, this->_decipherer);

        // DASH manifest handling
        std::string dashManifestUrl;
        auto dmu = streamingData["dashManifestUrl"];
        if(!dmu.is_null()) dashManifestUrl = dmu.get<std::string>();

        // Extract player source URL
        auto playerSourceUrl = _extractPlayerSourceURLFromRawSource(dl);

        d.resolve(playerSourceUrl, dashManifestUrl);
    })
    .then([=](const std::string &playerSourceUrl, const std::string &dashManifestUrl){
        return this->_downloadAndfillFrom_PlayerSource(playerSourceUrl)
        .then([=](){
            return dashManifestUrl;
        });
    })
    .then([=](const std::string &dashManifestUrl){
       auto mayFetchRawDASH = dashManifestUrl.empty() ? promise::resolve() : promise_dl_HTTPS(dashManifestUrl).then([=](const DownloadedUtf8 &dl) {
            streamsManifest->feedRaw_DASH(dl, this->_decipherer);
        });
        return mayFetchRawDASH;
    })
    .then([=]() {
        return *this;
    });
}

std::string AudioTube::PlayerConfig::_getSts(const DownloadedUtf8 &dl) {
    jp::VecNum matches;
    jp::RegexMatch rm;
    rm.setRegexObject(&Regexes::STSFinder)
        .setSubject(&dl)
        .addModifier("gm")
        .setNumberedSubstringVector(&matches)
        .match();

    // check if has STS
    if (!matches.size())
        throw std::logic_error("STS value cannot be found !");

    // returns first one
    return matches[0][0];
}
