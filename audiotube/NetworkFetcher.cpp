#include "NetworkFetcher.h"

promise::Defer NetworkFetcher::fromPlaylistUrl(const QString &url) {
    return download(url)
            .then(&_extractVideoIdsFromHTTPRequest)
            .then(&_videoIdsToMetadataList);         
};

promise::Defer NetworkFetcher::refreshMetadata(VideoMetadata* toRefresh, bool force) {
    
    //check if soft refresh...
    if(!force && toRefresh->isMetadataValid()) return promise::resolve(toRefresh);
    
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
        
        _getPlayerConfiguration(toRefresh) 
        .then([=](const PlayerConfiguration &pConfig) {
            return _fetchDecipherer(pConfig)
                .then([=](SignatureDecipherer* decipherer) {
                    auto audioStreams = pConfig.getUrlsByAudioStreams(decipherer);
                    toRefresh->setAudioStreamInfos(audioStreams);
                });
        })
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
        })
        .fail([=](){
            qWarning() << "AudioTube : unknown failure !";
            whenFailed();
        });

    });
};


void NetworkFetcher::isStreamAvailable(VideoMetadata* toCheck, bool* checkEnded, QString* urlSuccessfullyRequested) {
    
    auto streams = toCheck->audioStreams().second;
    auto firstUrl = streams.value(streams.uniqueKeys().first());
    
    download(firstUrl, true)
        .then([=](){
            *urlSuccessfullyRequested = firstUrl.toString();
        })        
        .finally([=](){
            *checkEnded = true;
        });

}

promise::Defer NetworkFetcher::_getPlayerConfiguration(VideoMetadata* metadata) {
    
    switch (metadata->preferedPlayerConfigFetchingMethod()) {  
        
        case VideoMetadata::PreferedPlayerConfigFetchingMethod::VideoInfo: {
            return _getPlayerConfiguration_VideoInfo(metadata);
        }
        break;
        
        case VideoMetadata::PreferedPlayerConfigFetchingMethod::WatchPage: {
            return _getPlayerConfiguration_WatchPage(metadata);
        }
        break;        
        
        case VideoMetadata::PreferedPlayerConfigFetchingMethod::Unknown: {
            return _getPlayerConfiguration_VideoInfo(metadata)
                   .fail([=](const QString &softErr){
                        qDebug() << qUtf8Printable(softErr);
                        return _getPlayerConfiguration_WatchPage(metadata);
                    });
        }
        break;

    }

}

promise::Defer NetworkFetcher::_getPlayerConfiguration_VideoInfo(VideoMetadata* metadata) {
    
    //mainly set player source URL to VideoMetadata
    auto videoId = metadata->id();
    auto embed = _getVideoEmbedPageHtml(videoId).then(_extractDataFrom_EmbedPageHtml);
    auto vinfos = _getVideoInfosDic(videoId).then(_extractDataFrom_VideoInfos);

    //handle augment
    return promise::all({ embed, vinfos }).then([=](const std::vector<promise::pm_any> &results) {

        //prepare args
        auto embed_Data  = results[0];
            auto playerSourceUrl = static_cast<QString*>(embed_Data.tuple_element(0));
            auto title = static_cast<QString*>(embed_Data.tuple_element(1));
            auto duration = static_cast<int*>(embed_Data.tuple_element(2));
        
        //prepare args
        auto vinfos_Data = results[1];
            auto expirationDate = static_cast<QDateTime*>(vinfos_Data.tuple_element(0));
            auto dashManifestUrl = static_cast<QString*>(vinfos_Data.tuple_element(1));
            auto streamInfos_UrlEncoded = static_cast<QString*>(vinfos_Data.tuple_element(2));
            auto streamInfos_JSON = static_cast<QJsonArray*>(vinfos_Data.tuple_element(3));

        //define pConfig
        PlayerConfiguration pConfig(
            *playerSourceUrl,
            *dashManifestUrl,
            *streamInfos_UrlEncoded,
            *streamInfos_JSON,
            *expirationDate
        );

        //update metadata
        metadata->setTitle(*title);
        metadata->setDuration(*duration);
        metadata->setExpirationDate(*expirationDate);
        metadata->setPreferedPlayerConfigFetchingMethod(VideoMetadata::PreferedPlayerConfigFetchingMethod::VideoInfo);

        return pConfig;

    });
}

promise::Defer NetworkFetcher::_getPlayerConfiguration_WatchPage(VideoMetadata* metadata) {
    return _getWatchPageHtml(metadata->id())
            .then(_extractDataFrom_WatchPage)
            .then([=](
                const QDateTime &expirationDate, 
                const QString &playerSourceUrl,
                const QString &title, 
                const int duration, 
                const QString &dashManifestUrl,
                const QString &streamInfos_UrlEncoded,
                const QJsonArray &streamInfos_JSON
            ){
                //define pConfig
                PlayerConfiguration pConfig(
                    playerSourceUrl,
                    dashManifestUrl,
                    streamInfos_UrlEncoded,
                    streamInfos_JSON,
                    expirationDate
                );

                //update metadata
                metadata->setTitle(title);
                metadata->setDuration(duration);
                metadata->setExpirationDate(expirationDate);
                metadata->setPreferedPlayerConfigFetchingMethod(VideoMetadata::PreferedPlayerConfigFetchingMethod::WatchPage);

                return pConfig;
            
            });
}


promise::Defer NetworkFetcher::_extractDataFrom_VideoInfos(const DownloadedUtf8 &dl, const QDateTime &requestedAt) {
    return promise::newPromise([=](promise::Defer d) {
 
        //as string then to query
        QUrlQuery videoInfos(dl);
        
        //get player response
        auto playerResponseAsStr = videoInfos.queryItemValue("player_response", QUrl::ComponentFormattingOption::FullyDecoded);
        auto playerResponse = QJsonDocument::fromJson(playerResponseAsStr.toUtf8());
        if(playerResponseAsStr.isEmpty() || playerResponse.isNull()) {
            throw std::logic_error("Player response is missing !");
        }

        //check if is live
        auto videoDetails = playerResponse[QStringLiteral(u"videoDetails")].toObject();
        auto isLiveStream = videoDetails.value(QStringLiteral(u"isLive")).toBool();
        if(isLiveStream) {
            throw std::logic_error("Live streams are not handled for now!");
        }

        //check playability status
        auto playabilityStatus = playerResponse[QStringLiteral(u"playabilityStatus")].toObject();
        auto pStatus = playabilityStatus.value(QStringLiteral(u"status")).toString();
        if(pStatus.toLower() == "error") {
            throw std::logic_error("This video is not available !");
        }

        //check reason, throw soft error
        auto pReason = playabilityStatus.value(QStringLiteral(u"reason")).toString();
        if(!pReason.isNull()) {
            throw QString(qUtf8Printable(QString("This video is not available though VideoInfo : %1").arg(pReason)));
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
        auto expirationDate = requestedAt.addSecs((qint64)expiresIn.toDouble());
        
        d.resolve(
            expirationDate,
            streamingData.value(QStringLiteral(u"dashManifestUrl")).toString(),
            videoInfos.queryItemValue(QStringLiteral(u"adaptive_fmts"), QUrl::ComponentFormattingOption::FullyDecoded),
            streamingData[QStringLiteral(u"adaptiveFormats")].toArray()
        );

    })
    .fail([=](QString &softErr) {
        return promise::reject(softErr);
    })
    .fail([=](std::logic_error &err) {
        return promise::reject(err);
    });
}

QString NetworkFetcher::_extractPlayerSourceUrlFromPlayerConfig(const QJsonObject &playerConfig) {
    auto playerSourceUrlPath = playerConfig[QStringLiteral(u"assets")].toObject()[QStringLiteral(u"js")].toString();
    if(playerSourceUrlPath.isEmpty()) throw std::logic_error("Player source URL is cannot be found !");
    return QStringLiteral("https://www.youtube.com") + playerSourceUrlPath;
}

QJsonObject NetworkFetcher::_extractPlayerConfigFromRawSource(const DownloadedUtf8 &rawSource, const QRegularExpression &regex) {
    
    auto playerConfigAsStr = regex.match(rawSource).captured("playerConfig");
    auto playerConfig = QJsonDocument::fromJson(playerConfigAsStr.toUtf8()).object();

    //check config exists
    if(playerConfigAsStr.isEmpty() || playerConfig.isEmpty()) {
        throw std::logic_error("Player response is missing !");
    }

    return playerConfig;

}

promise::Defer NetworkFetcher::_extractDataFrom_EmbedPageHtml(const DownloadedUtf8 &videoEmbedPageRequestData) {

    return promise::newPromise([=](promise::Defer d) {
        
        auto playerConfig = _extractPlayerConfigFromRawSource(
            videoEmbedPageRequestData,
            QRegularExpression("yt\\.setConfig\\({'PLAYER_CONFIG': (?<playerConfig>.*?)}\\);")
        );

        auto playerSourceUrl = _extractPlayerSourceUrlFromPlayerConfig(playerConfig);

        auto args = playerConfig[QStringLiteral(u"args")].toObject();
        auto title = args[QStringLiteral(u"title")].toString();
        auto videoLength = args[QStringLiteral(u"length_seconds")].toInt();

        if(title.isEmpty()) throw std::logic_error("Video title cannot be found !");
        if(!videoLength) throw std::logic_error("Video length cannot be found !");

        d.resolve(
            playerSourceUrl,
            title,
            videoLength
        );

    })
    .fail([=](std::logic_error &err) {
        return promise::reject(err);
    });
    
}

promise::Defer NetworkFetcher::_fetchDecipherer(const PlayerConfiguration &playerConfig) {
    return promise::newPromise([=](promise::Defer d){
        
        auto ytPlayerSourceUrl = playerConfig.playerSourceUrl();
        auto cachedDecipherer = SignatureDecipherer::fromCache(ytPlayerSourceUrl);

        if(cachedDecipherer) d.resolve(cachedDecipherer);
        else {
            download(ytPlayerSourceUrl)
            .then([=](const DownloadedUtf8 &dl){
                
                auto newDecipherer = SignatureDecipherer::create(
                    ytPlayerSourceUrl,
                    dl
                );

                d.resolve(newDecipherer);

            });
        }
    });
}

promise::Defer NetworkFetcher::_getWatchPageHtml(const VideoMetadata::Id &videoId) {
    auto url = QStringLiteral("https://www.youtube.com/watch?v=%1&bpctr=9999999999&hl=en").arg(videoId);
    auto requestedAt = QDateTime::currentDateTime();

    return promise::newPromise([=](promise::Defer d){
        download(url)
        .then([=](const DownloadedUtf8 &dlAsStr){
            d.resolve(dlAsStr, requestedAt); 
        });
    });
}

promise::Defer NetworkFetcher::_extractDataFrom_WatchPage(const DownloadedUtf8 &dl, const QDateTime &requestedAt) {
    
    return promise::newPromise([=](promise::Defer d) {

        //get player config JSON
        auto playerConfig =_extractPlayerConfigFromRawSource(
            dl,
            QRegularExpression(QStringLiteral("ytplayer\\.config = (?<playerConfig>.*?)\\;ytplayer"))
        );
        
        // Extract player source URL
        auto playerSourceUrl = _extractPlayerSourceUrlFromPlayerConfig(playerConfig);

        // Get player response JSON
        auto args = playerConfig[QStringLiteral(u"args")].toObject();
        auto playerResponseAsStr = args[QStringLiteral(u"player_response")].toString();
        auto playerResponse = QJsonDocument::fromJson(playerResponseAsStr.toUtf8());
        if(playerResponseAsStr.isEmpty() || playerResponse.isNull()) {
            throw std::logic_error("Player response is missing !");
        }

        //fetch and check video infos
        auto videoDetails = playerResponse[QStringLiteral(u"videoDetails")].toObject();
        auto title = videoDetails[QStringLiteral(u"title")].toString();
        auto duration = videoDetails[QStringLiteral(u"lengthSeconds")].toString().toInt();
        auto isLive = videoDetails[QStringLiteral(u"isLive")].toBool();

        if(isLive) throw std::logic_error("Live streams are not handled for now!");
        if(title.isEmpty()) throw std::logic_error("Video title cannot be found !");
        if(!duration) throw std::logic_error("Video length cannot be found !");

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
        auto expirationDate = requestedAt.addSecs((qint64)expiresIn.toDouble());

        d.resolve(
            expirationDate,
            playerSourceUrl,
            title,
            duration,
            streamingData.value(QStringLiteral(u"dashManifestUrl")).toString(),
            args.value(QStringLiteral(u"adaptive_fmts")).toString(),
            streamingData[QStringLiteral(u"adaptiveFormats")].toArray()
        );

    })
    .fail([=](std::logic_error &err) {
        return promise::reject(err);
    });

}

promise::Defer NetworkFetcher::_getVideoInfosDic(const VideoMetadata::Id &videoId) {

    auto apiUrl = QStringLiteral(u"https://youtube.googleapis.com/v/") + videoId;
    auto encodedApiUrl = QString::fromUtf8(QUrl::toPercentEncoding(apiUrl));

    auto requestUrl = QStringLiteral(u"https://www.youtube.com/get_video_info?video_id=%1&el=embedded&eurl=%2&hl=en&sts=18333")
        .arg(videoId)
        .arg(encodedApiUrl);

    auto requestedAt = QDateTime::currentDateTime();

    return promise::newPromise([=](promise::Defer d){
        download(requestUrl)
        .then([=](const DownloadedUtf8 &dlAsStr){
            d.resolve(dlAsStr, requestedAt); 
        });
    });

}


promise::Defer NetworkFetcher::_getVideoEmbedPageHtml(const VideoMetadata::Id &videoId) {
    auto videoEmbedPageHtmlUrl = QStringLiteral(u"https://www.youtube.com/embed/%1?hl=en").arg(videoId);
    return download(videoEmbedPageHtmlUrl);
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
        out.append(new VideoMetadata(id));
    } 
    return out;
};
