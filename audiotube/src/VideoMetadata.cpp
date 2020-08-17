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

#include "VideoMetadata.h"

AudioTube::VideoMetadata* AudioTube::VideoMetadata::fromVideoId(const std::string &videoId) {
    return new VideoMetadata(videoId, InstantiationType::InstFromId);
}

AudioTube::VideoMetadata* AudioTube::VideoMetadata::fromVideoUrl(const std::string &url) {
    return new VideoMetadata(url, InstantiationType::InstFromUrl);
}

void AudioTube::VideoMetadata::setOnMetadataFetching(const std::function<void()> &cb) {
    _omf_callback = cb;
}

void AudioTube::VideoMetadata::setOnMetadataRefreshed(const std::function<void()> &cb) {
    _omr_callback = cb;
}

void AudioTube::VideoMetadata::setOnStreamFailed(const std::function<void()> &cb) {
    _osf_callback = cb;
}

std::string AudioTube::VideoMetadata::_urlFromVideoId(const std::string &videoId) {
    return std::string("https://www.youtube.com/watch?v=") + videoId;
}

AudioTube::VideoMetadata::VideoMetadata(const std::string &IdOrUrl, const InstantiationType &type) {
    switch (type) {
        case InstantiationType::InstFromUrl: {
            // find id
            std::smatch idMatches;
            std::regex_search(IdOrUrl, idMatches, Regexes::YoutubeIdFinder);

            // returns
            if (idMatches.size() != 1) {
                throw std::invalid_argument("URL is not a valid  URL !");
            }

            this->_videoId = idMatches.str(0);
            this->_url = IdOrUrl;
        }
        break;

        case InstantiationType::InstFromId: {
            this->_videoId = IdOrUrl;
            this->_url = _urlFromVideoId(IdOrUrl);
        }
        break;
    }
}

AudioTube::PlayerConfig::VideoId AudioTube::VideoMetadata::id() const {
    return this->_videoId;
}

std::string AudioTube::VideoMetadata::url() const {
    return this->_url;
}

bool AudioTube::VideoMetadata::hasFailed() const {
    return this->_failed;
}

void AudioTube::VideoMetadata::setRanOnce() {
    this->_ranOnce = true;
}

bool AudioTube::VideoMetadata::ranOnce() const {
    return this->_ranOnce;
}

void AudioTube::VideoMetadata::setFailure(bool failed) {
    if (failed == true && this->_failed != failed) OnStreamFailed();
    this->_failed = failed;
}

void AudioTube::VideoMetadata::setPlayerConfig(const PlayerConfig &playerConfig) {
    this->_playerConfig = playerConfig;
}

AudioTube::StreamsManifest* AudioTube::VideoMetadata::audioStreams() {
    return &this->_audioStreams;
}

AudioTube::PlayerConfig* AudioTube::VideoMetadata::playerConfig() {
    return &this->_playerConfig;
}
