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

#include "StreamsManifest.h"

AudioTube::StreamsManifest::StreamsManifest() {}

void AudioTube::StreamsManifest::feedRaw_DASH(const RawDASHManifest &raw, const SignatureDecipherer* decipherer) {
    // find streams
    auto foundStreams = Regexes::DASHManifestExtractor.globalMatch(raw);

    // container
    AudioStreamUrlByBitrate streams;

    // iterate
    while (foundStreams.hasNext()) {
        auto match = foundStreams.next();

        // check codec
        auto codec = match.captured("codec");
        if (!_isCodecAllowed(codec)) continue;

        auto itag = match.captured("itag").toInt();
        auto url = match.captured("url");
        auto bitrate = match.captured("bitrate").toDouble();

        streams.insert(bitrate, { itag, url });
    }

    // insert in package
    this->_package.insert(AudioStreamsSource::DASH, streams);
}

void AudioTube::StreamsManifest::feedRaw_PlayerConfig(const RawPlayerConfigStreams &raw, const SignatureDecipherer* decipherer) {
    // TODO(amphaal)
}

void AudioTube::StreamsManifest::feedRaw_PlayerResponse(const RawPlayerResponseStreams &raw, const SignatureDecipherer* decipherer) {
    AudioStreamUrlByBitrate streams;

    // iterate
    for (auto itagGroup : raw) {
        auto itagGroupObj = itagGroup.toObject();

        // check mime
        auto mimeType = itagGroupObj["mimeType"].toString();
        if (!_isMimeAllowed(mimeType)) continue;

        // find itag + url
        auto bitrate = itagGroupObj["bitrate"].toDouble();
        auto url = itagGroupObj["url"].toString();
        auto tag = itagGroupObj["itag"].toInt();

        // decipher if no url
        if (url.isEmpty()) {
            // find cipher
            auto cipherRaw = itagGroupObj["cipher"].toString();
            if (cipherRaw.isEmpty()) cipherRaw = itagGroupObj["signatureCipher"].toString();
            if (cipherRaw.isEmpty()) throw std::logic_error("Cipher data cannot be found !");

            auto cipher = QUrlQuery(cipherRaw);

            // find params
            auto cipheredUrl = cipher.queryItemValue("url", QUrl::ComponentFormattingOption::FullyDecoded);
            auto signature = cipher.queryItemValue("s", QUrl::ComponentFormattingOption::FullyDecoded);
            auto signatureParameter = cipher.queryItemValue("sp", QUrl::ComponentFormattingOption::FullyDecoded);

            // decipher
            url = _decipheredUrl(
                decipherer,
                cipheredUrl,
                signature,
                signatureParameter
            );
        }

        // add tag / url pair
        streams.insert(bitrate, { tag, url });
    }

    // insert in package
    this->_package.insert(AudioStreamsSource::PlayerResponse, streams);
}

QString AudioTube::StreamsManifest::_decipheredUrl(const SignatureDecipherer* decipherer, const QString &cipheredUrl, QString signature, QString sigKey) {
    QString out = cipheredUrl;

    // find signature param, set default if empty
    if (sigKey.isEmpty()) sigKey = "signature";

    // decipher...
    signature = decipherer->decipher(signature);

    // append
    out += QString("&%1=%2")
            .arg(sigKey)
            .arg(signature);
    return out;
}

void AudioTube::StreamsManifest::setRequestedAt(const QDateTime &requestedAt) {
    this->_requestedAt = requestedAt;
}
void AudioTube::StreamsManifest::setSecondsUntilExpiration(const uint secsUntilExp) {
    this->_validUntil = this->_requestedAt.addSecs(secsUntilExp);
}

QPair<AudioTube::StreamsManifest::AudioStreamsSource, AudioTube::StreamsManifest::AudioStreamUrlByBitrate> AudioTube::StreamsManifest::preferedStreamSource() const {
    // sort sources
    auto sources = this->_package.keys();
    std::sort(sources.begin(), sources.end());

    // try to fetch in order of preference
    for (auto source : sources) {
        auto ASUBIT = this->_package.value(source);
        if (ASUBIT.count()) return { source, ASUBIT };
    }

    throw std::logic_error("No audio stream source found !");
}

QUrl AudioTube::StreamsManifest::preferedUrl() const {
    auto source = this->preferedStreamSource();
    qDebug() << "Picking stream URL from source : " << qUtf8Printable(QVariant::fromValue(source.first).toString());
    return source.second.last().second;  // since bitrates are asc-ordered, take latest for fastest
}

bool AudioTube::StreamsManifest::isExpired() const {
    if (this->_validUntil.isNull()) return true;
    return QDateTime::currentDateTime() > this->_validUntil;
}


bool AudioTube::StreamsManifest::_isCodecAllowed(const QString &codec) {
    if (codec.contains(QStringLiteral(u"opus"))) return true;
    return false;
}

bool AudioTube::StreamsManifest::_isMimeAllowed(const QString &mime) {
    if (!mime.contains(QStringLiteral(u"audio"))) return false;
    return _isCodecAllowed(mime);
}

QJsonArray AudioTube::StreamsManifest::_urlEncodedToJsonArray(const QString &urlQueryAsRawStr) {
    QJsonArray out;

    // for each group
    auto itagsDataGroupsAsStr = urlQueryAsRawStr.split(
        QStringLiteral(u","),
        Qt::SkipEmptyParts
    );
    for (auto &dataGroupAsString : itagsDataGroupsAsStr) {
        QJsonObject group;

        // for each pair
        auto pairs = dataGroupAsString.split(
            QStringLiteral(u"&"),
            Qt::SkipEmptyParts
        );

        for (const auto &pair : pairs) {
            // current key/value pair
            auto kvpAsList = pair.split(
                QStringLiteral(u"="),
                Qt::KeepEmptyParts
            );
            auto kvp = QPair<QString, QString>(kvpAsList.at(0), kvpAsList.at(1));

            // add to temporary group
            group.insert(
                kvp.first,
                QUrlQuery(kvp.second).toString(QUrl::ComponentFormattingOption::FullyDecoded)
            );
        }

        out.append(group);
    }

    return out;
}
