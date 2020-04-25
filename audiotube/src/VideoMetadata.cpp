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

VideoMetadata* VideoMetadata::fromVideoId(const QString &videoId) {
    return new VideoMetadata(videoId, InstantiationType::InstFromId);
}

VideoMetadata* VideoMetadata::fromVideoUrl(const QString &url) {
    return new VideoMetadata(url, InstantiationType::InstFromUrl);
}

QString VideoMetadata::_urlFromVideoId(const QString &videoId) {
    return QStringLiteral(u"https://www.youtube.com/watch?v=") + videoId;
}

VideoMetadata::VideoMetadata(const QString &IdOrUrl, const InstantiationType &type) {
    switch (type) {
        case InstantiationType::InstFromUrl: {
            // find id
            auto match = Regexes::YoutubeIdFinder.match(IdOrUrl);

            // returns
            if (!match.hasMatch()) {
                throw std::invalid_argument("URL is not a valid  URL !");
            }

            this->_videoId = match.captured("videoId");
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

PlayerConfig::VideoId VideoMetadata::id() const {
    return this->_videoId;
}

QString VideoMetadata::url() const {
    return this->_url;
}

bool VideoMetadata::hasFailed() const {
    return this->_failed;
}

void VideoMetadata::setRanOnce() {
    this->_ranOnce = true;
}

bool VideoMetadata::ranOnce() const {
    return this->_ranOnce;
}

void VideoMetadata::setFailure(bool failed) {
    if (failed == true && this->_failed != failed) emit streamFailed();
    this->_failed = failed;
}

void VideoMetadata::setPlayerConfig(const PlayerConfig &playerConfig) {
    this->_playerConfig = playerConfig;
}

StreamsManifest* VideoMetadata::audioStreams() {
    return &this->_audioStreams;
}

const PlayerConfig& VideoMetadata::playerConfig() const {
    return this->_playerConfig;
}
