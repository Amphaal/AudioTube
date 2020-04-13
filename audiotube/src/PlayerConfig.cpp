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

PlayerConfig::PlayerConfig(const VideoMetadata::PreferedStreamContextSource &streamContextSource, const VideoMetadata::Id &videoId) {
    this->_contextSource = streamContextSource;
    this->_videoId = videoId;
    this->_requestedAt = QDateTime::currentDateTime();
}

promise::Defer PlayerConfig::from(const VideoMetadata::PreferedStreamContextSource &streamContextSource, const VideoMetadata::Id &videoId) {
    
    PlayerConfig pConfig(streamContextSource, videoId);

    switch(streamContextSource) {

        case VideoMetadata::PreferedStreamContextSource::VideoInfo:
            return _from_VideoInfo(pConfig);
        break;

        case VideoMetadata::PreferedStreamContextSource::WatchPage:
            return _from_WatchPage(pConfig);
        break;

    }

}
    
promise::Defer PlayerConfig::_from_VideoInfo(PlayerConfig &pConfig) {
    //TODO
}

promise::Defer PlayerConfig::_from_WatchPage(PlayerConfig &pConfig) {
    //TODO
}

promise::Defer PlayerConfig::_downloadRaw_VideoEmbedPageHtml(const VideoMetadata::Id &videoId) {
    auto url = QStringLiteral(u"https://www.youtube.com/embed/%1?hl=en").arg(videoId);
    return download(url);
}

promise::Defer PlayerConfig::_downloadRaw_WatchPageHtml(const VideoMetadata::Id &videoId) {
    auto url = QStringLiteral("https://www.youtube.com/watch?v=%1&bpctr=9999999999&hl=en").arg(videoId);
    return download(url);
}


QJsonObject PlayerConfig::_extractPlayerConfigFromRawSource(const DownloadedUtf8 &rawSource, const QRegularExpression &regex) {

    auto playerConfigAsStr = regex.match(rawSource).captured("playerConfig");
    auto playerConfig = QJsonDocument::fromJson(playerConfigAsStr.toUtf8()).object();

    //check config exists
    if(playerConfigAsStr.isEmpty() || playerConfig.isEmpty()) {
        throw std::logic_error("Player response is missing !");
    }

    return playerConfig;

}

QString PlayerConfig::_playerSourceUrl(const QJsonObject &playerConfig) {
    auto playerSourceUrlPath = playerConfig[QStringLiteral(u"assets")].toObject()[QStringLiteral(u"js")].toString();
    if(playerSourceUrlPath.isEmpty()) throw std::logic_error("Player source URL is cannot be found !");
    return QStringLiteral("https://www.youtube.com") + playerSourceUrlPath;
}

promise::Defer PlayerConfig::_downloadAndfillFrom_PlayerSource(const QString &playerSourceUrl) {
    return promise::newPromise([=](promise::Defer d){
        
        //if cached is found, return it
        auto cachedDecipherer = SignatureDecipherer::fromCache(playerSourceUrl);
        if(cachedDecipherer) {
            this->_decipherer = cachedDecipherer;
        }

        //if no cached decipherer or if STS is needed
        auto stsNeeded = this->_contextSource == VideoMetadata::PreferedStreamContextSource::VideoInfo;
        if(!cachedDecipherer || stsNeeded) {
            download(playerSourceUrl)
            .then([=](const DownloadedUtf8 &dl) {
                
                //generate decipherer
                if(!cachedDecipherer) {
                    this->_decipherer = SignatureDecipherer::create(
                        playerSourceUrl,
                        dl
                    );
                }

                //fetch STS
                if(stsNeeded) {
                    this->_sts = _sts(dl);
                }

                //resolve at last
                d.resolve();

            });
        }
        
        //else resolve immediately
        d.resolve();

    });
}

promise::Defer PlayerConfig::_fillFrom_WatchPageHtml(const DownloadedUtf8 &dl) {
    
    return promise::newPromise([=](promise::Defer d) {

        //get player config JSON
        auto playerConfig =_extractPlayerConfigFromRawSource(dl,
            QRegularExpression(QStringLiteral("ytplayer\\.config = (?<playerConfig>.*?)\\;ytplayer"))
        );

        // Get player response JSON
        auto args = playerConfig[QStringLiteral(u"args")].toObject();
        auto playerResponseAsStr = args[QStringLiteral(u"player_response")].toString();
        auto playerResponse = QJsonDocument::fromJson(playerResponseAsStr.toUtf8());
        if(playerResponseAsStr.isEmpty() || playerResponse.isNull()) {
            throw std::logic_error("Player response is missing !");
        }

        //fetch and check video infos
        auto videoDetails = playerResponse[QStringLiteral(u"videoDetails")].toObject();
        this->_title = videoDetails[QStringLiteral(u"title")].toString();
        this->_duration = videoDetails[QStringLiteral(u"lengthSeconds")].toString().toInt();
        auto isLive = videoDetails[QStringLiteral(u"isLive")].toBool();

        if(isLive) throw std::logic_error("Live streams are not handled for now!");
        if(this->_title.isEmpty()) throw std::logic_error("Video title cannot be found !");
        if(!this->_duration) throw std::logic_error("Video length cannot be found !");

        //check playability status, throw err
        auto playabilityStatus = playerResponse[QStringLiteral(u"playabilityStatus")].toObject();
        auto pReason = playabilityStatus.value(QStringLiteral(u"reason")).toString();
        if(!pReason.isNull()) {
            throw std::logic_error("This video is not available though WatchPage : %1");
        }

        //get streamingData
        auto streamingData = playerResponse[QStringLiteral(u"streamingData")].toObject();
        if(streamingData.isEmpty()) {
            throw std::logic_error("An error occured while fetching video infos");
        }

        //find expiration
        auto expiresIn = streamingData.value(QStringLiteral(u"expiresInSeconds")).toString();
        if(expiresIn.isEmpty()) {
            throw std::logic_error("An error occured while fetching video infos");
        }

        //set expiration date
        this->_expireAt = this->_requestedAt.addSecs((qint64)expiresIn.toDouble());

        //DASH manifest URL
        auto dashManifestUrl = streamingData.value(QStringLiteral(u"dashManifestUrl")).toString();

        // Extract player source URL
        auto playerSourceUrl = _playerSourceUrl(playerConfig);
        _downloadAndfillFrom_PlayerSource(playerSourceUrl).then([=]() {
            
            //TODO 
            auto raw_playerConfigStreams = args.value(QStringLiteral(u"adaptive_fmts")).toString();
            auto raw_playerResponseStreams = streamingData[QStringLiteral(u"adaptiveFormats")].toArray();

        });

    });

}

int PlayerConfig::_sts(const DownloadedUtf8 &dl) {
    //TODO
}