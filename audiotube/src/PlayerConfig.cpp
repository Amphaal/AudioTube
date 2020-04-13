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

PlayerConfig::PlayerConfig() {}

PlayerConfig::PlayerConfig(const PlayerConfig::ContextSource &streamContextSource, const VideoMetadata::Id &videoId) {
    this->_contextSource = streamContextSource;
}

QString PlayerConfig::sts() const {
    return this->_sts;
}

int PlayerConfig::duration() const {
    return this->_duration;
}

QString PlayerConfig::title() const {
    return this->_title;
}

const SignatureDecipherer* PlayerConfig::decipherer() const {
    return this->_decipherer;
}

PlayerConfig::ContextSource PlayerConfig::contextSource() const {
    return this->_contextSource;
}

promise::Defer PlayerConfig::from_VideoInfo(const VideoMetadata::Id &videoId) {
    
    //inst
    PlayerConfig pConfig(PlayerConfig::ContextSource::VideoInfo, videoId);
    
    //pipeline
    return _downloadRaw_VideoEmbedPageHtml(videoId)
            .then([=](const DownloadedUtf8 &dl) mutable {
                return pConfig._fillFrom_VideoEmbedPageHtml(dl);
            })
            .then([=]() {
                return pConfig;
            });

}

promise::Defer PlayerConfig::from_WatchPage(const VideoMetadata::Id &videoId, StreamsManifest &streamsManifest) {
    
    //inst
    PlayerConfig pConfig(PlayerConfig::ContextSource::WatchPage, videoId);

    //define requested at timestamp
    streamsManifest.setRequestedAt(QDateTime::currentDateTime());

    //pipeline
    return _downloadRaw_WatchPageHtml(videoId)
            .then([=](const DownloadedUtf8 &dl) mutable {
                return pConfig._fillFrom_WatchPageHtml(dl, streamsManifest);
            })
            .then([=]() {
                return pConfig;
            });

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
        auto stsNeeded = this->_contextSource == PlayerConfig::ContextSource::VideoInfo;
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

promise::Defer PlayerConfig::_fillFrom_VideoEmbedPageHtml(const DownloadedUtf8 &dl) {
    return promise::newPromise([=](promise::Defer d) {
        
        auto playerConfig = _extractPlayerConfigFromRawSource(dl,
            QRegularExpression("yt\\.setConfig\\({'PLAYER_CONFIG': (?<playerConfig>.*?)}\\);")
        );

        //get title and duration
        auto args = playerConfig[QStringLiteral(u"args")].toObject();
        this->_title = args[QStringLiteral(u"title")].toString();
        this->_duration = args[QStringLiteral(u"length_seconds")].toInt();

        if(this->_title.isEmpty()) throw std::logic_error("Video title cannot be found !");
        if(!this->_duration) throw std::logic_error("Video length cannot be found !");

        //player source
        auto playerSourceUrl = _playerSourceUrl(playerConfig);
        return _downloadAndfillFrom_PlayerSource(playerSourceUrl);

    });
}

promise::Defer PlayerConfig::_fillFrom_WatchPageHtml(const DownloadedUtf8 &dl, StreamsManifest &streamsManifest) {
    
    return promise::newPromise([=](promise::Defer d) mutable {

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
        streamsManifest.setSecondsUntilExpiration((qint64)expiresIn.toDouble()); 

        //DASH manifest handling
        auto dashManifestUrl = streamingData.value(QStringLiteral(u"dashManifestUrl")).toString();
        auto mayFetchRawDASH = dashManifestUrl.isEmpty() ? promise::resolve() : download(dashManifestUrl).then([=](const DownloadedUtf8 &dl) mutable {
            streamsManifest.feedRaw_DASH(dl, this->_decipherer);
        });

        //raw stream infos
        auto raw_playerConfigStreams = args.value(QStringLiteral(u"adaptive_fmts")).toString();
        auto raw_playerResponseStreams = streamingData[QStringLiteral(u"adaptiveFormats")].toArray();

        //feed
        streamsManifest.feedRaw_PlayerConfig(raw_playerConfigStreams, this->_decipherer);
        streamsManifest.feedRaw_PlayerResponse(raw_playerResponseStreams, this->_decipherer);

        // Extract player source URL
        auto playerSourceUrl = _playerSourceUrl(playerConfig);
        return mayFetchRawDASH.then([=]() {
            return _downloadAndfillFrom_PlayerSource(playerSourceUrl);
        });

    });

}

QString PlayerConfig::_sts(const DownloadedUtf8 &dl) {
    //TODO
}