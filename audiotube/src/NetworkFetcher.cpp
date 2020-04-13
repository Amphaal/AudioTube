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

promise::Defer NetworkFetcher::fromPlaylistUrl(const QString &url) {
    return download(url)
            .then(&_extractVideoIdsFromHTTPRequest)
            .then(&_videoIdsToMetadataList);         
};

promise::Defer NetworkFetcher::refreshMetadata(VideoMetadata* toRefresh, bool force) {
    
    //check if soft refresh...
    if(!force && !toRefresh->audioStreams().isExpired()) return promise::resolve(toRefresh);
    
    //if not, reset failure flag and emit event
    toRefresh->setFailure(false);
    emit toRefresh->metadataFetching();

    //workflow...
    return promise::newPromise([=](promise::Defer d){
        
        //on error default behavior 
        auto whenFailed = [=]() {
           toRefresh->setFailure(true);
           toRefresh->setRanOnce(); 
           d.reject();
        };
        
        _refreshMetadata(toRefresh) 
        .then([=]() {
            //success !
            toRefresh->setRanOnce();
            emit toRefresh->metadataRefreshed();
            d.resolve(toRefresh);
        })
        .fail([=](const std::runtime_error &exception) {
            qWarning() << "AudioTube :" << exception.what();     
            whenFailed();
        })
        .fail([=](const std::logic_error &exception) {
            qWarning() << "AudioTube :" << exception.what();     
            whenFailed();
        });

    });
};

void NetworkFetcher::isStreamAvailable(VideoMetadata* toCheck, bool* checkEnded, QString* urlSuccessfullyRequested) {
    
    auto bestUrl = toCheck->audioStreams().preferedUrl();
    
    download(bestUrl, true)
        .then([=](){
            *urlSuccessfullyRequested = bestUrl.toString();
        })        
        .finally([=](){
            *checkEnded = true;
        });

}

promise::Defer NetworkFetcher::_refreshMetadata(VideoMetadata* metadata) {
    
    //if player config has never been fetched
    if(metadata->playerConfig().contextSource() == PlayerConfig::ContextSource::Unknown) {

        PlayerConfig::from_VideoInfo(metadata->id());
        PlayerConfig::from_WatchPage(metadata->id(), metadata->audioStreams());

    }
        
        case PlayerConfig::ContextSource::VideoInfo: {
            return _getStreamContext_VideoInfo(metadata);
        }
        break;
        
        case PlayerConfig::ContextSource::WatchPage: {
            return _getStreamContext_WatchPage(metadata);
        }
        break;        
        
        case PlayerConfig::ContextSource::Unknown: {
            return _getStreamContext_VideoInfo(metadata)
                   .fail([=](const QString &softErr){
                        qDebug() << qUtf8Printable(softErr);
                        return _getStreamContext_WatchPage(metadata);
                    });
        }
        break;

    }

    return promise::resolve();

}

QList<QString> NetworkFetcher::_extractVideoIdsFromHTTPRequest(const DownloadedUtf8 &requestData) {

    //build regex
    QRegularExpression re("watch\\?v=(?<videoId>.*?)&amp;");

    //search...
    auto results = re.globalMatch(requestData);
    
    //return list
    QList<QString> idsList;

    //iterate
    while (results.hasNext()) {
        QRegularExpressionMatch match = results.next(); //next
        
        //if match
        if (match.hasMatch()) {
            auto found = match.captured("videoId"); //videoId
            
            if(!idsList.contains(found)) {
                idsList.append(found);
            }
        }
    }

    //if no ids
    if(!idsList.count()) throw std::logic_error("no playlist metadata container found !");

    //return
    return idsList;

}

QList<VideoMetadata*> NetworkFetcher::_videoIdsToMetadataList(const QList<QString> &videoIds) {
    QList<VideoMetadata*> out;
    for(const auto &id : videoIds) {
        out.append(VideoMetadata::fromVideoId(id));
    } 
    return out;
};
