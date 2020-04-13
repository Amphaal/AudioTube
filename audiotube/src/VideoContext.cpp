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

#include "VideoContext.h"

VideoContext::VideoContext(
    const QString &playerSourceUrl, 
    const QString &dashManifestUrl, 
    const QString &adaptiveStreamInfosUrlEncoded, 
    const QJsonArray &adaptiveStreamInfosJson, 
    const QDateTime &validUntil
) {
    _playerSourceUrl = playerSourceUrl;
    _dashManifestUrl = dashManifestUrl;
    _adaptiveStreamInfosUrlEncoded = _urlEncodedToJsonArray(adaptiveStreamInfosUrlEncoded);
    _adaptiveStreamInfosJson = adaptiveStreamInfosJson;
    _validUntil = validUntil;

    adaptiveStreamAsUrlEncoded = !_adaptiveStreamInfosUrlEncoded.isEmpty();
    adaptiveStreamAsJson = _adaptiveStreamInfosJson.count();
    adaptiveStreamAsDash = !_dashManifestUrl.isEmpty();
    
    if(!adaptiveStreamAsDash && !adaptiveStreamAsUrlEncoded && !adaptiveStreamAsJson) 
        throw std::logic_error("Stream infos cannot be fetched !");
}

VideoContext::AudioStreamsPackage VideoContext::getUrlsByAudioStreams(const SignatureDecipherer* dcfrer) const {
    if(adaptiveStreamAsDash) return getUrlsByAudioStreams_DASH(dcfrer);
    if(adaptiveStreamAsJson) return getUrlsByAudioStreams_JSON(dcfrer);
    if(adaptiveStreamAsUrlEncoded) return getUrlsByAudioStreams_UrlEncoded(dcfrer);
    throw std::logic_error("Unhandled Stream infos type !");
}

QString VideoContext::playerSourceUrl() const {
    return _playerSourceUrl;
}

//Dash manifest deciphering not handled yet ! //TODO
QString VideoContext::decipherDashManifestUrl(const SignatureDecipherer* dcfrer) const {
    return _dashManifestUrl;
}

void VideoContext::fillRawDashManifest(const RawDashManifest &rawDashManifest) {
    _rawDashManifest = rawDashManifest;
}

VideoContext::AudioStreamsPackage VideoContext::getUrlsByAudioStreams_UrlEncoded(const SignatureDecipherer* dcfrer) const {
    AudioStreamsPackage out;
    out.first = PreferedAudioStreamsInfosSource::UrlEncoded;
    
    throw std::runtime_error("Stream info format not handled yet !"); //TODO
}

VideoContext::AudioStreamsPackage VideoContext::getUrlsByAudioStreams_DASH(const SignatureDecipherer* dcfrer) const {
    
    //check if raw data is here
    if(_rawDashManifest.isEmpty())
        throw std::runtime_error("Dash Manifest is empty !");

    //find streams
    QRegularExpression regex("<Representation id=\"(?<itag>.*?)\" codecs=\"(?<codec>.*?)\"[\\s\\S]*?<BaseURL>(?<url>.*?)<\\/BaseURL");
    auto foundStreams = regex.globalMatch(_rawDashManifest);
    
    //container
    AudioStreamsPackage out; 
    out.first = PreferedAudioStreamsInfosSource::DASH;

    //iterate
    while(foundStreams.hasNext()) {
        auto match = foundStreams.next();
        
        //check codec
        auto codec = match.captured("codec");
        if(!_isCodecAllowed(codec)) continue;

        auto itag = match.captured("itag").toInt();
        auto url = match.captured("url");

        out.second.insert(itag, url);
    }

    return out;

}

VideoContext::AudioStreamsPackage VideoContext::getUrlsByAudioStreams_JSON(const SignatureDecipherer* dcfrer) const {
    
    AudioStreamsPackage out;
    out.first = PreferedAudioStreamsInfosSource::JSON;

    for(auto itagGroup : _adaptiveStreamInfosJson) {
        auto itagGroupObj = itagGroup.toObject();

        uint itag = itagGroupObj["itag"].toInt();
        if(itag != 18) continue;

        // auto mimeType = itagGroupObj["mimeType"].toString();
        // if(!_isMimeAllowed(mimeType)) continue;

        auto url = itagGroupObj["url"].toString();

        //decipher
        if(url.isEmpty()) {
            
            auto cipher = QUrlQuery(itagGroupObj["cipher"].toString());
            url = cipher.queryItemValue("url", QUrl::ComponentFormattingOption::FullyDecoded);

            qDebug() << url;
            auto signature = cipher.queryItemValue("s", QUrl::ComponentFormattingOption::FullyDecoded);
            
            //find signature param
            auto signatureParameter = cipher.queryItemValue("sp", QUrl::ComponentFormattingOption::FullyDecoded);
            if(signatureParameter.isEmpty()) signatureParameter = "signature";

            //decipher...
            signature = dcfrer->decipher(signature);
            qDebug() << signature;

            //append
            url += QString("&%1=%2").arg(signatureParameter).arg(signature);

        }


        out.second.insert(itag, url);

    }

    return out;

}

bool VideoContext::_isCodecAllowed(const QString &codec) {
    if(codec.contains(QStringLiteral(u"opus"))) return true;
    return false;
}

bool VideoContext::_isMimeAllowed(const QString &mime) {
    if(!mime.contains(QStringLiteral(u"audio"))) return false;
    return _isCodecAllowed(mime);
}

QJsonArray VideoContext::_urlEncodedToJsonArray(const QString &urlQueryAsRawStr) {

    QJsonArray out;

    //for each group
    auto itagsDataGroupsAsStr = urlQueryAsRawStr.split(
        QStringLiteral(u","), 
        QString::SplitBehavior::SkipEmptyParts
    );
    for(auto &dataGroupAsString : itagsDataGroupsAsStr) {

        QJsonObject group;
        
        //for each pair
        auto pairs = dataGroupAsString.split(
            QStringLiteral(u"&"), 
            QString::SplitBehavior::SkipEmptyParts
        );

        for(const auto &pair : pairs) {
            
            //current key/value pair
            auto kvpAsList = pair.split(
                QStringLiteral(u"="), 
                QString::SplitBehavior::KeepEmptyParts
            );
            auto kvp = QPair<QString, QString>(kvpAsList.at(0), kvpAsList.at(1));

            //add to temporary group
            group.insert(
                kvp.first, 
                QUrlQuery(kvp.second).toString(QUrl::ComponentFormattingOption::FullyDecoded)
            );

        }

        out.append(group);

    }

    return out;

}