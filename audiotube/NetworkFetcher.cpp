#include "NetworkFetcher.h"

promise::Defer NetworkFetcher::fromPlaylistUrl(const QString &url) {
    return download(url)
            .then(&_extractVideoIdsFromHTTPRequest)
            .then(&_videoIdsToMetadataList);         
};

void NetworkFetcher::isStreamAvailable(VideoMetadata* toCheck, bool* checkEnded, bool* success) {
    
    auto streams = toCheck->audioStreams();
    auto firstUrl = streams.value(streams.uniqueKeys().first());
    
    download(firstUrl, true)
        .then([=](){
            *success = true;
        })        
        .fail([=](){
            *success = false;
        })
        .finally([=](){
            *checkEnded = true;
        });

}

promise::Defer NetworkFetcher::refreshMetadata(VideoMetadata* toRefresh, bool force) {
    
    if(!force && toRefresh->isMetadataValid()) return promise::resolve(toRefresh);
    
    toRefresh->setFailure(false);
    emit toRefresh->metadataFetching();

    return promise::newPromise([=](promise::Defer d){
        
        auto whenFailed = [=]() {
           toRefresh->setFailure(true);
           toRefresh->setRanOnce(); 
           d.reject();
        };

        _getVideoEmbedPageRawData(toRefresh)
        .then([toRefresh](const QByteArray &htmlResponse) {
            return _augmentMetadataWithPlayerConfiguration(toRefresh, htmlResponse);
        })
        .then(&_downloadVideoInfosAndAugmentMetadata)
        .then([=]() {
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


promise::Defer NetworkFetcher::_getVideoEmbedPageRawData(VideoMetadata* metadata) {
    auto videoEmbedPageHtmlUrl = QStringLiteral(u"https://www.youtube.com/embed/%1?hl=en").arg(metadata->id());
    return download(videoEmbedPageHtmlUrl);
}

VideoMetadata* NetworkFetcher::_augmentMetadataWithPlayerConfiguration(VideoMetadata* metadata, const QByteArray &videoEmbedPageRequestData) {
    
    //to string for pcre2 usage
    auto str = QString::fromUtf8(videoEmbedPageRequestData);

    //extract player config
    QRegularExpression re("yt\\.setConfig\\({'PLAYER_CONFIG': (?<playerConfig>.*?)}\\);");
    auto playerConfigAsStr = re.match(str).captured("playerConfig");
    auto playerConfig = QJsonDocument::fromJson(playerConfigAsStr.toUtf8()).object();
    auto args = playerConfig[QStringLiteral(u"args")].toObject();

    //get valuable data from it
    auto playerSourceUrl = playerConfig[QStringLiteral(u"assets")].toObject()[QStringLiteral(u"js")].toString();
    auto title = args[QStringLiteral(u"title")].toString();
    auto videoLength = args[QStringLiteral(u"length_seconds")].toInt();

    //check values exist
    if(playerSourceUrl.isEmpty()) throw std::logic_error("Player source URL is cannot be found !");
    if(title.isEmpty()) throw std::logic_error("Video title cannot be found !");
    if(!videoLength) throw std::logic_error("Video length cannot be found !");

    //augment...
    metadata->setPlayerSourceUrl("https://www.youtube.com" + playerSourceUrl);
    metadata->setTitle(title);
    metadata->setDuration(videoLength);

    return metadata;
    
}

VideoMetadata* NetworkFetcher::_augmentMetadataWithVideoInfos(
    VideoMetadata* metadata,
    SignatureDecipherer* decipherer,
    const QByteArray &videoInfoRawResponse, 
    const QDateTime &tsRequest
) {
        
    //as string then to query, check for errors
    QUrlQuery videoInfos(QString::fromUtf8(videoInfoRawResponse));
    auto error = videoInfos.queryItemValue("errorcode");
    auto status = videoInfos.queryItemValue("status");
    if(!error.isNull() || status != "ok") {
        throw std::logic_error("Video is not available !");
    }

    //player response as JSON, check if error
    auto playerResponseAsStr = videoInfos.queryItemValue("player_response", QUrl::ComponentFormattingOption::FullyDecoded);
    auto playerResponse = QJsonDocument::fromJson(playerResponseAsStr.toUtf8());
    if(playerResponseAsStr.isEmpty() || playerResponse.isNull()) {
        throw std::logic_error("Player response is missing !");
    }

    //check playability status
    auto playabilityStatus = playerResponse[QStringLiteral(u"playabilityStatus")].toObject();
    auto pStatus = playabilityStatus.value(QStringLiteral(u"reason")).toString();
    if(!pStatus.isNull()) {
        throw std::logic_error(qUtf8Printable(QString("Video is not available ! %1").arg(pStatus)));
    }

    //check if is live
    auto videoDetails = playerResponse[QStringLiteral(u"videoDetails")].toObject();
    auto isLiveStream = videoDetails.value(QStringLiteral(u"isLive")).toBool();
    if(isLiveStream) throw std::logic_error("Live streams are not handled for now!");

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
    auto expirationDate = tsRequest.addSecs((qint64)expiresIn.toDouble());
    metadata->setExpirationDate(expirationDate);

    PlayerConfiguration pConfig(
        metadata->playerSourceUrl(),
        streamingData.value(QStringLiteral(u"dashManifestUrl")).toString(),
        streamingData.value(QStringLiteral(u"hlsManifestUrl")).toString(),
        videoInfos.queryItemValue("adaptive_fmts", QUrl::ComponentFormattingOption::FullyDecoded),
        streamingData["adaptiveFormats"].toArray(),
        expirationDate
    );

    //define stream infos    
    metadata->setAudioStreamInfos(pConfig.getUrlsByAudioStreams(decipherer));

    return metadata;
    
}

promise::Defer NetworkFetcher::_downloadVideoInfosAndAugmentMetadata(VideoMetadata* metadata) {
    
    //define timestamp for request
    auto requestedAt = QDateTime::currentDateTime();
    auto ytPlayerSourceUrl = metadata->playerSourceUrl();
    auto cachedDecipherer = SignatureDecipherer::fromCache(ytPlayerSourceUrl);

    // qDebug() << ": YT player source URL : " + ytPlayerSourceUrl;

    //helper for raw data download
    QVector<promise::Defer> dlPromises{
        _getVideoInfosRawData(metadata),
        !cachedDecipherer ? download(ytPlayerSourceUrl) : promise::resolve(QByteArray())
    };
    auto downloadAll = promise::all(dlPromises);

    //handle augment
    return downloadAll.then([=](const std::vector<promise::pm_any> &results) {

        //prepare args
        auto videoInfosRawData = static_cast<QByteArray*>(results[0].tuple_element(0));
        auto rawPlayerSourceData = static_cast<QByteArray*>(results[1].tuple_element(0));

        auto c_decipherer = cachedDecipherer ? cachedDecipherer : SignatureDecipherer::create(
                                                                    ytPlayerSourceUrl,
                                                                    QString::fromUtf8(*rawPlayerSourceData)
                                                                  );

        //augment
        return _augmentMetadataWithVideoInfos(metadata, c_decipherer, *videoInfosRawData, requestedAt);

    });
}

QString NetworkFetcher::_getApiUrl(const QString &videoId) {
    return QStringLiteral(u"https://youtube.googleapis.com/v/") + videoId;
}

promise::Defer NetworkFetcher::_getVideoInfosRawData(VideoMetadata* metadata) {
    
    auto id = metadata->id();
    auto apiUrl = _getApiUrl(id);
    
    auto encodedApiUrl = QString::fromUtf8(
        QUrl::toPercentEncoding(apiUrl)
    );

    auto requestUrl = QStringLiteral(u"https://www.youtube.com/get_video_info?video_id=%1&el=embedded&eurl=%2&hl=en&sts=18333")
        .arg(id)
        .arg(encodedApiUrl);
    
    return download(requestUrl);

}


QList<QString> NetworkFetcher::_extractVideoIdsFromHTTPRequest(const QByteArray &requestData) {
        
        //to string for pcre2 usage
        auto str = QString::fromUtf8(requestData);

        //build regex
        QRegularExpression re("watch\\?v=(?<videoId>.*?)&amp;");

        //search...
        auto results = re.globalMatch(str);
        
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
