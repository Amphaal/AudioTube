#include "YoutubeHelper.h"

promise::Defer YoutubeHelper::fromPlaylistUrl(const QString &url) {
    return download(url)
            .then(&_extractVideoIdsFromHTTPRequest)
            .then(&_videoIdsToMetadataList);         
};

promise::Defer YoutubeHelper::refreshMetadata(YoutubeVideoMetadata* toRefresh, bool force) {
    
    if(!force && toRefresh->isMetadataValid()) return promise::resolve(toRefresh);
    
    toRefresh->setFailure(false);
    emit toRefresh->metadataFetching();

    return promise::newPromise([=](promise::Defer d){
        
        _getVideoEmbedPageRawData(toRefresh)
        .then([toRefresh](const QByteArray &htmlResponse) {
            return _augmentMetadataWithPlayerConfiguration(toRefresh, htmlResponse);
        })
        .then(&_downloadVideoInfosAndAugmentMetadata)
        .then([=]() {
            emit toRefresh->metadataRefreshed();
            d.resolve(toRefresh);
        }).fail([=](const std::runtime_error &err) {
            qWarning() << "Youtube : error while fecthing audio stream >> " << err.what();
            toRefresh->setFailure(true); 
        }).fail([=]() {
           toRefresh->setFailure(true); 
        });

    });
};


promise::Defer YoutubeHelper::_getVideoEmbedPageRawData(YoutubeVideoMetadata* metadata) {
    auto videoEmbedPageHtmlUrl = QStringLiteral(u"https://www.youtube.com/embed/%1?disable_polymer=true&hl=en").arg(metadata->id());
    return download(videoEmbedPageHtmlUrl);
}

YoutubeVideoMetadata* YoutubeHelper::_augmentMetadataWithPlayerConfiguration(YoutubeVideoMetadata* metadata, const QByteArray &videoEmbedPageRequestData) {
    
    //to string for pcre2 usage
    auto str = QString::fromUtf8(videoEmbedPageRequestData);

    //extract player config
    QRegularExpression re("yt\\.setConfig\\({'PLAYER_CONFIG': (?<playerConfig>.*?)}\\);");
    auto playerConfigAsStr = re.match(str).captured("playerConfig");
    auto playerConfig = QJsonDocument::fromJson(playerConfigAsStr.toUtf8()).object();
    auto args = playerConfig[QStringLiteral(u"args")].toObject();

    //get valuable data from it
    auto sts = args[QStringLiteral(u"cver")].toString();
    auto playerSourceUrl = playerConfig[QStringLiteral(u"assets")].toObject()[QStringLiteral(u"js")].toString();
    auto title = args[QStringLiteral(u"title")].toString();
    auto length = args[QStringLiteral(u"length_seconds")].toInt();

    //check values exist
    if(sts.isEmpty() || playerSourceUrl.isEmpty() || title.isEmpty() || !length) {
        throw new std::logic_error("error while getting player client configuration !");
    }

    //augment...
    metadata->setSts(sts);
    metadata->setPlayerSourceUrl("https://www.youtube.com" + playerSourceUrl);
    metadata->setTitle(title);
    metadata->setDuration(length);

    return metadata;
    
}

YoutubeVideoMetadata* YoutubeHelper::_augmentMetadataWithVideoInfos(
    YoutubeVideoMetadata* metadata,
    YoutubeSignatureDecipherer* decipherer,
    const QByteArray &videoInfoRawResponse, 
    const QDateTime &tsRequest
) {
        
    //as string then to query, check for errors
    QUrlQuery videoInfos(QString::fromUtf8(videoInfoRawResponse));
    auto error = videoInfos.queryItemValue("errorcode");
    auto status = videoInfos.queryItemValue("status");
    if(!error.isNull() || status != "ok") {
        throw new std::logic_error("An error occured while fetching video infos");
    }

    //player response as JSON, check if error
    auto playerResponseAsStr = videoInfos.queryItemValue("player_response", QUrl::ComponentFormattingOption::FullyDecoded);
    auto playerResponse = QJsonDocument::fromJson(playerResponseAsStr.toUtf8());
    if(playerResponseAsStr.isEmpty() || playerResponse.isNull()) {
        throw new std::logic_error("An error occured while fetching video infos");
    }

    //check playability status
    auto playabilityStatus = playerResponse[QStringLiteral(u"playabilityStatus")].toObject();
    auto pStatus = playabilityStatus.value(QStringLiteral(u"reason")).toString();
    if(!pStatus.isNull()) {
        throw new std::logic_error("An error occured while fetching video infos");
    }

    //get streamingData
    auto streamingData = playerResponse[QStringLiteral(u"streamingData")].toObject();
    if(streamingData.isEmpty()) {
        throw new std::logic_error("An error occured while fetching video infos");
    }

    //find expiration
    auto expiresIn = streamingData.value(QStringLiteral(u"expiresInSeconds")).toString();
    if(expiresIn.isEmpty()) {
        throw new std::logic_error("An error occured while fetching video infos");
    }

    //set expiration date
    auto expirationDate =  tsRequest.addSecs((qint64)expiresIn.toDouble());
    metadata->setExpirationDate(expirationDate);

    //find stream infos
    YoutubeAudioStreamInfos streamInfos;

        //...from video infos
        auto streamInfosAsStr = videoInfos.queryItemValue("adaptive_fmts", QUrl::ComponentFormattingOption::FullyDecoded);
        if(!streamInfosAsStr.isEmpty()) {
            streamInfos = YoutubeAudioStreamInfos(decipherer, streamInfosAsStr);
        } 
        
        //...from streaming data
        else {
            
            //try...
            auto streamInfosAsJSONArray = streamingData["adaptiveFormats"].toArray();
            if(!streamInfosAsJSONArray.isEmpty()) {
                streamInfos = YoutubeAudioStreamInfos(streamInfosAsJSONArray);
            }

            //failed...
            else {
                throw new std::logic_error("An error occured while fetching video infos");
            }

        }

    //define stream infos    
    metadata->setAudioStreamInfos(streamInfos);

    return metadata;
    
}

void YoutubeHelper::_dumpAsJSON(const QUrlQuery &query) {
    
    QJsonObject dump;
    
    for(const auto &item : query.queryItems(QUrl::ComponentFormattingOption::FullyDecoded)) {
        dump.insert(item.first, item.second);
    }

    _dumpAsJSON(dump);

}

void YoutubeHelper::_dumpAsJSON(const QJsonObject &obj) {
    return _dumpAsJSON(QJsonDocument(obj));
}

void YoutubeHelper::_dumpAsJSON(const QJsonArray &arr) {
    return _dumpAsJSON(QJsonDocument(arr));
}

void YoutubeHelper::_dumpAsJSON(const QJsonDocument &doc) {
    
    auto bytes = doc.toJson(QJsonDocument::JsonFormat::Indented);

    QFile fh("yt.json");
    fh.open(QFile::WriteOnly);
        fh.write(bytes);
    fh.close();

    qDebug() << fh.fileName() << " created !";

}

promise::Defer YoutubeHelper::_downloadVideoInfosAndAugmentMetadata(YoutubeVideoMetadata* metadata) {
    
    //define timestamp for request
    auto requestedAt = QDateTime::currentDateTime();
    auto ytPlayerSourceUrl = metadata->playerSourceUrl();
    auto cachedDecipherer = YoutubeSignatureDecipherer::fromCache(ytPlayerSourceUrl);

    // qDebug() << "Youtube: YT player source URL : " + ytPlayerSourceUrl;

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

        auto c_decipherer = cachedDecipherer ? cachedDecipherer : YoutubeSignatureDecipherer::create(
                                                                    ytPlayerSourceUrl,
                                                                    QString::fromUtf8(*rawPlayerSourceData)
                                                                  );

        //augment
        return _augmentMetadataWithVideoInfos(metadata, c_decipherer, *videoInfosRawData, requestedAt);

    });
}

QString YoutubeHelper::_getApiUrl(const QString &videoId) {
    return QStringLiteral(u"https://youtube.googleapis.com/v/") + videoId;
}

promise::Defer YoutubeHelper::_getVideoInfosRawData(YoutubeVideoMetadata* metadata) {
    
    auto id = metadata->id();
    auto apiUrl = _getApiUrl(id);
    
    auto encodedApiUrl = QString::fromUtf8(
        QUrl::toPercentEncoding(apiUrl)
    );

    auto requestUrl = QStringLiteral(u"https://www.youtube.com/get_video_info?video_id=%1&el=embedded&sts=%2&eurl=%3&hl=en")
        .arg(id)
        .arg(metadata->sts())
        .arg(encodedApiUrl);
    
    return download(requestUrl);

}


QList<QString> YoutubeHelper::_extractVideoIdsFromHTTPRequest(const QByteArray &requestData) {
        
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
        if(!idsList.count()) throw new std::logic_error("no playlist metadata container found !");

        //return
        return idsList;
}

QList<YoutubeVideoMetadata*> YoutubeHelper::_videoIdsToMetadataList(const QList<QString> &videoIds) {
    QList<YoutubeVideoMetadata*> out;
    for(const auto &id : videoIds) {
        out.append(new YoutubeVideoMetadata(id));
    } 
    return out;
};
