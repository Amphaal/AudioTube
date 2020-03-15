#include "YoutubeAudioStreamInfos.h"

YoutubeAudioStreamInfos::YoutubeAudioStreamInfos() {};
YoutubeAudioStreamInfos::YoutubeAudioStreamInfos(YoutubeSignatureDecipherer* decipherer, const QString &urlQueryAsRawStr) {
    auto raw = _generatRawAdaptiveStreamInfosFromUrlQuery(urlQueryAsRawStr);
    this->_initFromUrlQuery(decipherer, raw);
}

YoutubeAudioStreamInfos::YoutubeAudioStreamInfos(const QJsonArray &adaptativeFormatsWithUnsignedUrls) {
    
    auto raw = _generatRawAdaptiveStreamInfosFromJSON(adaptativeFormatsWithUnsignedUrls);
    
    //filter, only audio
    for(auto &streamInfos : raw) {
        auto type = streamInfos.value(QStringLiteral(u"mimeType"));
        
        //exclude
        if(type.isEmpty()) continue;
        if(!type.contains(QStringLiteral(u"audio"))) continue;

        //add to internal
        this->_InfosByAudioMime.insert(type, streamInfos);
    }

}

void YoutubeAudioStreamInfos::_initFromUrlQuery(YoutubeSignatureDecipherer* decipherer, const RawInfosByAudioMime &rawData) {

    //filter, only audio
    for(auto &streamInfos : rawData) {
        auto type = streamInfos.value(QStringLiteral(u"type"));
        
        //exclude
        if(type.isEmpty()) continue;
        if(!type.contains(QStringLiteral(u"audio"))) continue;

        //add to internal
        this->_InfosByAudioMime.insert(type, streamInfos);
    }

    //alter url parameter if signature is needed
    for(auto &audioStreamInfos : this->_InfosByAudioMime) { 
        
        //signature
        auto baseSignature = audioStreamInfos.value(QStringLiteral(u"s"));
        if(baseSignature.isEmpty()) continue;

        //modify signature
        auto newSignature = decipherer->decipher(baseSignature);
        audioStreamInfos.insert(
            QStringLiteral(u"s_deciphered"), 
            newSignature
        );

        //update url accordingly
        auto signatureParam = audioStreamInfos.value(QStringLiteral(u"sp"), QStringLiteral(u"signature"));
        auto baseUrl = QUrl(audioStreamInfos.value(QStringLiteral(u"url")));

            auto baseUrl_query = QUrlQuery(baseUrl.query());
            baseUrl_query.addQueryItem(signatureParam, newSignature);
            baseUrl.setQuery(baseUrl_query);

        audioStreamInfos.insert(
            QStringLiteral(u"url"), 
            baseUrl.toString()
        );

    }

}

const YoutubeAudioStreamInfos::UrlMimePair YoutubeAudioStreamInfos::getPreferedMimeSourcePair() const {
    auto available = this->availableAudioMimes();
    auto mp4Audio = available.filter(QRegularExpression(QStringLiteral(u"opus")));
    auto selectedMime = mp4Audio.count() ? mp4Audio.value(0) : available.value(0);
    auto selectedUrl = this->streamUrl(selectedMime);
    return {selectedMime, selectedUrl};
}

const QString YoutubeAudioStreamInfos::streamUrl(const QString &mime) const {
    return this->_InfosByAudioMime
                    .value(mime)
                    .value(QStringLiteral(u"url"));
}

const QList<QString> YoutubeAudioStreamInfos::availableAudioMimes() const {
    return this->_InfosByAudioMime.keys();
}

YoutubeAudioStreamInfos::RawInfosByAudioMime YoutubeAudioStreamInfos::_generatRawAdaptiveStreamInfosFromJSON(const QJsonArray &jsonArray) {
    
    RawInfosByAudioMime out;

    for(auto i = 0; i < jsonArray.count(); i++) {

        QHash<QString, QString> group;

        auto q = jsonArray.at(i).toObject().toVariantHash();
        for(auto i = q.begin(); i != q.end(); i++) {
            if(i.value().userType() != QMetaType::QString) continue;
            group.insert(i.key(), i.value().toString());
        }

        out += group;

    }

    return out;

}

YoutubeAudioStreamInfos::RawInfosByAudioMime YoutubeAudioStreamInfos::_generatRawAdaptiveStreamInfosFromUrlQuery(const QString &urlQueryAsRawStr) {

    RawInfosByAudioMime out;

    //for each group
    auto itagsDataGroupsAsStr = urlQueryAsRawStr.split(
        QStringLiteral(u","), 
        QString::SplitBehavior::SkipEmptyParts
    );
    for(auto &dataGroupAsString : itagsDataGroupsAsStr) {

        QHash<QString, QString> group;
        
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
