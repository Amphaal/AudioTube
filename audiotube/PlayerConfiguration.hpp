#pragma once

#include <QString>
#include <QJsonArray>
#include <QDateTime>
#include <QJsonObject>
#include <QVariantHash>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>

#include "_base/_DebugHelper.h"
#include "SignatureDecipherer.h"

class PlayerConfiguration {
    public:
        enum PreferedAudioStreamsInfosSource {
            Unknown,
            DASH, 
            UrlEncoded,
            JSON
        };
        using RawDashManifest = QString;
        
        using AudioStreamUrlByITag = QPair<PreferedAudioStreamsInfosSource, QHash<uint, QUrl>>;
        PlayerConfiguration(
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
    
    AudioStreamUrlByITag getUrlsByAudioStreams(const SignatureDecipherer* dcfrer) const {
        if(adaptiveStreamAsDash) return getUrlsByAudioStreams_DASH(dcfrer);
        if(adaptiveStreamAsJson) return getUrlsByAudioStreams_JSON(dcfrer);
        if(adaptiveStreamAsUrlEncoded) return getUrlsByAudioStreams_UrlEncoded(dcfrer);
        throw std::logic_error("Unhandled Stream infos type !");
    }

    QString playerSourceUrl() const {
        return _playerSourceUrl;
    }

    //Dash manifest deciphering not handled yet ! //TODO
    QString decipherDashManifestUrl(const SignatureDecipherer* dcfrer) const {
        return _dashManifestUrl;
    }

    void fillRawDashManifest(const RawDashManifest &rawDashManifest) {
        _rawDashManifest = rawDashManifest;
    }

    private:
        QString _playerSourceUrl;
        QString _dashManifestUrl;
        RawDashManifest _rawDashManifest;
        QJsonArray _adaptiveStreamInfosUrlEncoded;
        QJsonArray _adaptiveStreamInfosJson;
        QDateTime _validUntil;

        bool adaptiveStreamAsUrlEncoded = false;
        bool adaptiveStreamAsJson = false;
        bool adaptiveStreamAsDash = false;

        AudioStreamUrlByITag getUrlsByAudioStreams_UrlEncoded(const SignatureDecipherer* dcfrer) const {
            throw std::runtime_error("Stream info format not handled yet !"); //TODO
        }

        AudioStreamUrlByITag getUrlsByAudioStreams_DASH(const SignatureDecipherer* dcfrer) const {
            
            //check if raw data is here
            if(_rawDashManifest.isEmpty())
                throw std::runtime_error("Dash Manifest is empty !");

            //find streams
            QRegularExpression regex("<Representation id=\"(?<itag>.*?)\" codecs=\"(?<codec>.*?)\"[\\s\\S]*?<BaseURL>(?<url>.*?)<\\/BaseURL");
            auto foundStreams = regex.globalMatch(_rawDashManifest);
            
            //container
            AudioStreamUrlByITag out; 
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

        AudioStreamUrlByITag getUrlsByAudioStreams_JSON(const SignatureDecipherer* dcfrer) const {
            
            AudioStreamUrlByITag out;
            out.first = PreferedAudioStreamsInfosSource::JSON;

            for(auto itagGroup : _adaptiveStreamInfosJson) {
                auto itagGroupObj = itagGroup.toObject();

                auto mimeType = itagGroupObj["mimeType"].toString();
                if(!_isMimeAllowed(mimeType)) continue;

                uint itag = itagGroupObj["itag"].toInt();
                auto url = itagGroupObj["url"].toString();

                //decipher
                if(url.isEmpty()) {
                    
                    auto cipher = QUrlQuery(itagGroupObj["cipher"].toString());
                    url = cipher.queryItemValue("url", QUrl::ComponentFormattingOption::FullyDecoded);

                    auto signature = cipher.queryItemValue("s", QUrl::ComponentFormattingOption::FullyDecoded);
                    auto signatureParameter = cipher.queryItemValue("sp", QUrl::ComponentFormattingOption::FullyDecoded);
                    if(signatureParameter.isEmpty()) signatureParameter = "signature";

                    signature = dcfrer->decipher(signature);
                    
                    url += QString("&%1=%2").arg(signatureParameter).arg(signature);
                }


                out.second.insert(itag, url);

            }

            return out;

        }

        static bool _isCodecAllowed(const QString &codec) {
            if(codec.contains(QStringLiteral(u"opus"))) return true;
            return false;
        }

        static bool _isMimeAllowed(const QString &mime) {
            if(!mime.contains(QStringLiteral(u"audio"))) return false;
            return _isCodecAllowed(mime);
        }

        static QJsonArray _urlEncodedToJsonArray(const QString &urlQueryAsRawStr) {

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

};