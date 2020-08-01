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
            auto cipheredUrl = cipher.queryItemValue("url", Url::ComponentFormattingOption::FullyDecoded);
            auto signature = cipher.queryItemValue("s", Url::ComponentFormattingOption::FullyDecoded);
            auto signatureParameter = cipher.queryItemValue("sp", Url::ComponentFormattingOption::FullyDecoded);

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

std::string AudioTube::StreamsManifest::_decipheredUrl(const SignatureDecipherer* decipherer, const std::string &cipheredUrl, std::string signature, std::string sigKey) {
    std::string out = cipheredUrl;

    // find signature param, set default if empty
    if (sigKey.isEmpty()) sigKey = "signature";

    // decipher...
    signature = decipherer->decipher(signature);

    // append
    out += std::string("&%1=%2")
            .arg(sigKey)
            .arg(signature);
    return out;
}

void AudioTube::StreamsManifest::setRequestedAt(const std::time_t &requestedAt) {
    this->_requestedAt = requestedAt;
}
void AudioTube::StreamsManifest::setSecondsUntilExpiration(const unsigned int secsUntilExp) {
    this->_validUntil = this->_requestedAt.addSecs(secsUntilExp);
}

std::pair<AudioTube::StreamsManifest::AudioStreamsSource, AudioTube::StreamsManifest::AudioStreamUrlByBitrate> AudioTube::StreamsManifest::preferedStreamSource() const {
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

std::string AudioTube::StreamsManifest::preferedUrl() const {
    auto source = this->preferedStreamSource();
    spdlog::debug("Picking stream URL from source : {}", std::variant::fromValue(source.first).toString());
    return source.second.last().second;  // since bitrates are asc-ordered, take latest for fastest
}

bool AudioTube::StreamsManifest::isExpired() const {
    if (this->_validUntil.isNull()) return true;
    return std::time_t::currentDateTime() > this->_validUntil;
}


bool AudioTube::StreamsManifest::_isCodecAllowed(const std::string &codec) {
    if (codec.contains(std::string(u"opus"))) return true;
    return false;
}

bool AudioTube::StreamsManifest::_isMimeAllowed(const std::string &mime) {
    if (!mime.contains(std::string(u"audio"))) return false;
    return _isCodecAllowed(mime);
}

QJsonArray AudioTube::StreamsManifest::_urlEncodedToJsonArray(const std::string &urlQueryAsRawStr) {
    QJsonArray out;

    // for each group
    auto itagsDataGroupsAsStr = urlQueryAsRawStr.split(
        std::string(u","),
        Qt::SkipEmptyParts
    );
    for (auto &dataGroupAsString : itagsDataGroupsAsStr) {
        QJsonObject group;

        // for each pair
        auto pairs = dataGroupAsString.split(
            std::string(u"&"),
            Qt::SkipEmptyParts
        );

        for (const auto &pair : pairs) {
            // current key/value pair
            auto kvpAsList = pair.split(
                std::string(u"="),
                Qt::KeepEmptyParts
            );
            auto kvp = std::pair<std::string, std::string>(kvpAsList.at(0), kvpAsList.at(1));

            // add to temporary group
            group.insert(
                kvp.first,
                QUrlQuery(kvp.second).toString(Url::ComponentFormattingOption::FullyDecoded)
            );
        }

        out.append(group);
    }

    return out;
}
