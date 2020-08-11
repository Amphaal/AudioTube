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
        auto itag = safe_stoi(streamDataMatch.str(0));
        auto codec = streamDataMatch.str(1);
        auto bitrate = safe_stoi(streamDataMatch.str(2));
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

            UrlQuery cipher(cipherRaw);

            // find params
            auto cipheredUrl = cipher["url"].percentDecoded();
            auto signature = cipher["s"].percentDecoded();
            auto signatureParameter = cipher["sp"].percentDecoded();

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
    // try to fetch in order of preference (sorted by enum)
    for (auto i = this->_package.begin(); i != this->_package.end(); i++) {
        auto source = i->first;
        auto ASUBIT = i->second;
        if (ASUBIT.size()) return { source, ASUBIT };
    }

    throw std::logic_error("No audio stream source found !");
}

std::string AudioTube::StreamsManifest::preferedUrl() const {
    auto source = this->preferedStreamSource();
    spdlog::debug("Picking stream URL from source : {}", source.first);
    return source.second.end()->second.second;  // since bitrates are asc-ordered, take latest for fastest
}

bool AudioTube::StreamsManifest::isExpired() const {
    if (this->_validUntil == -1) return true;

    auto now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );

    return now > this->_validUntil;
}


bool AudioTube::StreamsManifest::_isCodecAllowed(const std::string &codec) {
    return codec.find("opus") > std::string::npos;
}

bool AudioTube::StreamsManifest::_isMimeAllowed(const std::string &mime) {
    if (mime.find("audio") == std::string::npos) return false;
    return _isCodecAllowed(mime);
}
