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
    std::smatch foundStreams;
    std::regex_match(raw, foundStreams, Regexes::DASHManifestExtractor);

    // check
    if (!foundStreams.ready()) throw std::logic_error("[DASH] No stream found on manifest");

    // container
    AudioStreamUrlByBitrate streams;

    // iterate through streams
    for (auto &streamMatch : foundStreams) {
        // search data parts
        auto streamRawStr = streamMatch.str();
        std::smatch streamDataMatch;
        std::regex_search(streamRawStr, streamDataMatch, Regexes::DASHManifestExtractor);

        // check
        if (!streamDataMatch.ready() || streamDataMatch.size() != 4) throw std::logic_error("[DASH] Expected dataparts are missing from stream");

        // get parts
        auto itag = std::stoi(streamDataMatch.str(0));
        auto codec = streamDataMatch.str(1);
        auto bitrate = std::stod(streamDataMatch.str(2));
        auto url = streamDataMatch.str(3);

        // check codec
        if (!_isCodecAllowed(codec)) continue;

        // fill
        streams.emplace(bitrate, std::make_pair(itag, url));
    }

    // insert in package
    this->_package.emplace(AudioStreamsSource::DASH, streams);
}

void AudioTube::StreamsManifest::feedRaw_PlayerConfig(const RawPlayerConfigStreams &raw, const SignatureDecipherer* decipherer) {
    // TODO(amphaal)
}

void AudioTube::StreamsManifest::feedRaw_PlayerResponse(const RawPlayerResponseStreams &raw, const SignatureDecipherer* decipherer) {
    AudioStreamUrlByBitrate streams;

    // iterate
    for (auto itagGroup : raw) {
        // check mime
        auto mimeType = itagGroup["mimeType"].get<std::string>();
        if (!_isMimeAllowed(mimeType)) continue;

        // find itag + url
        auto bitrate = itagGroup["bitrate"].get<double>();
        auto url = itagGroup["url"].get<std::string>();
        auto tag = itagGroup["itag"].get<int>();

        // decipher if no url
        if (url.empty()) {
            // find cipher
            auto cipherRaw = itagGroup["cipher"].get<std::string>();
            if (cipherRaw.empty()) cipherRaw = itagGroup["signatureCipher"].get<std::string>();
            if (cipherRaw.empty()) throw std::logic_error("Cipher data cannot be found !");

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
        streams.emplace(bitrate, std::make_pair(tag, url));
    }

    // insert in package
    this->_package.emplace(AudioStreamsSource::PlayerResponse, streams);
}

std::string AudioTube::StreamsManifest::_decipheredUrl(const SignatureDecipherer* decipherer, const std::string &cipheredUrl, std::string signature, std::string sigKey) {
    std::string out = cipheredUrl;

    // find signature param, set default if empty
    if (sigKey.empty()) sigKey = "signature";

    // decipher...
    signature = decipherer->decipher(signature);

    // append
    out += std::string("&") + sigKey + "=" + signature;

    return out;
}

void AudioTube::StreamsManifest::setRequestedAt(const std::time_t &requestedAt) {
    this->_requestedAt = requestedAt;
}
void AudioTube::StreamsManifest::setSecondsUntilExpiration(const unsigned int secsUntilExp) {
    this->_validUntil = this->_requestedAt + secsUntilExp;
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
    if (this->_validUntil == -1) return true;

    auto now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );

    return now > this->_validUntil;
}


bool AudioTube::StreamsManifest::_isCodecAllowed(const std::string &codec) {
    return codec.find("opus") > -1;
}

bool AudioTube::StreamsManifest::_isMimeAllowed(const std::string &mime) {
    if (mime.find("audio") == -1) return false;
    return _isCodecAllowed(mime);
}

nlohmann::json AudioTube::StreamsManifest::_urlEncodedToJsonArray(const std::string &urlQueryAsRawStr) {
    nlohmann::json out;

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

        out.emplace(group);
    }

    return out;
}
