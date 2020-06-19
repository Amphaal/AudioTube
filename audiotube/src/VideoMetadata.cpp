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

AudioTube::VideoMetadata* AudioTube::VideoMetadata::fromVideoId(const QString &videoId) {
    return new VideoMetadata(videoId, InstantiationType::InstFromId);
}

AudioTube::VideoMetadata* AudioTube::VideoMetadata::fromVideoUrl(const QString &url) {
    return new VideoMetadata(url, InstantiationType::InstFromUrl);
}

QString AudioTube::VideoMetadata::_urlFromVideoId(const QString &videoId) {
    return QStringLiteral(u"https://www.youtube.com/watch?v=") + videoId;
}

AudioTube::VideoMetadata::VideoMetadata(const QString &IdOrUrl, const InstantiationType &type) {
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

AudioTube::PlayerConfig::VideoId AudioTube::VideoMetadata::id() const {
    return this->_videoId;
}

QString AudioTube::VideoMetadata::url() const {
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
    if (failed == true && this->_failed != failed) emit streamFailed();
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
