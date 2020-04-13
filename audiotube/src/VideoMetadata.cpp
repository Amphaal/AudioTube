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

QRegularExpression VideoMetadata::getUrlMatcher() {
    return _ytRegexIdFinder;
}

VideoMetadata* VideoMetadata::fromVideoId(const QString &videoId) {
    return new VideoMetadata(videoId, InstantiationType::InstFromId);
}

VideoMetadata* VideoMetadata::fromVideoUrl(const QString &url) {
    return new VideoMetadata(url, InstantiationType::InstFromUrl);
}

void VideoMetadata::setPreferedStreamContextSource(const VideoMetadata::PreferedStreamContextSource &method) {
    this->_preferedStreamContextSource = method;
}

VideoMetadata::PreferedStreamContextSource VideoMetadata::preferedStreamContextSource() const {
    return this->_preferedStreamContextSource;
}

VideoContext::PreferedAudioStreamsInfosSource VideoMetadata::preferedAudioStreamsInfosSource() const {
    return this->_preferedAudioStreamsInfosSource;
}

QUrl VideoMetadata::getBestAvailableStreamUrl() const {
    
    //sort itags [first is best]
    auto itags = this->_audioStreamInfos.uniqueKeys();
    std::sort(itags.begin(), itags.end());

    return this->_audioStreamInfos.value(itags.first());

}

QString VideoMetadata::_urlFromVideoId(const QString &videoId) {
    return QStringLiteral(u"https://www.youtube.com/watch?v=") + videoId;
}

VideoMetadata::VideoMetadata(const QString &IdOrUrl, const InstantiationType &type) {
    switch(type) {
        case InstantiationType::InstFromUrl: {
            //find id
            auto match = _ytRegexIdFinder.match(IdOrUrl);

            //returns
            if(!match.hasMatch()) {
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

VideoMetadata::Id VideoMetadata::id() const {
    return this->_videoId;
}

QString VideoMetadata::title() const {
    return this->_title;
}

QString VideoMetadata::url() const {
    return this->_url;
}

int VideoMetadata::duration() const {
    return this->_durationInSeconds;
}

bool VideoMetadata::isMetadataValid() const {
    if(this->_validUntil.isNull()) return false;
    return QDateTime::currentDateTime() < this->_validUntil;
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
    if(failed == true && this->_failed != failed) emit streamFailed();
    this->_failed = failed;
}

void VideoMetadata::setTitle(const QString &title) {
    this->_title = title;
}

void VideoMetadata::setDuration(int durationInSeconds) {
    this->_durationInSeconds = durationInSeconds;
}

void VideoMetadata::setExpirationDate(const QDateTime &expiration) {
    this->_validUntil = expiration;
}

void VideoMetadata::setAudioStreamsPackage(const VideoContext::AudioStreamsPackage &streamInfos) {
    
    if(streamInfos.first == VideoContext::PreferedAudioStreamsInfosSource::Unknown || !streamInfos.second.count()) 
        throw std::logic_error("Setting empty audio stream Infos is not allowed !");
    
    this->_audioStreamInfos = streamInfos.second;
    this->_preferedAudioStreamsInfosSource = streamInfos.first;

}

const VideoContext::AudioStreamUrlByITag& VideoMetadata::audioStreams() const {
    return this->_audioStreamInfos;
}