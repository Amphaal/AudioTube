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
    jp::VecNum matches;
    jp::RegexMatch rm;
    rm.setRegexObject(&Regexes::DASHManifestExtractor)
        .setSubject(&raw)
        .addModifier("gm")
        .setNumberedSubstringVector(&matches)
        .match();

    // check
    if (!matches.size()) throw std::logic_error("[DASH] No stream found on manifest");

    // container
    AudioStreamUrlByBitrate streams;

    // iterate through streams
    for (auto &submatches : matches) {
        // check
        if (submatches.size() != 5) throw std::logic_error("[DASH] Expected dataparts are missing from stream");

        // get parts
        auto itag = safe_stoi(submatches[1]);
        auto codec = submatches[2];
        auto bitrate = safe_stoi(submatches[3]);
        auto url = submatches[4];

        // check codec
        if (!_isCodecAllowed(codec)) continue;

        // fill
        streams.emplace(bitrate, std::make_pair(itag, url));
    }

    if(!streams.size()) return;

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
        auto tag = itagGroup["itag"].get<int>();
        auto bitrate = itagGroup["bitrate"].get<double>();
        std::string url;
        auto url_JSON = itagGroup["url"];

        // decipher if no url
        if (url_JSON.is_null()) {
            // find cipher
            auto cipherRaw = itagGroup["cipher"];
            if (cipherRaw.is_null()) cipherRaw = itagGroup["signatureCipher"];
            if (cipherRaw.is_null()) throw std::logic_error("Cipher data cannot be found !");

            auto cipherRawStr = cipherRaw.get<std::string>();
            UrlQuery cipher(cipherRawStr);

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
        } else {
            url = url_JSON.get<std::string>();
            spdlog::debug("PlayerResponse : Unciphered URL [{}]", url);
        }

        // add tag / url pair
        streams.emplace(bitrate, std::make_pair(tag, url));
    }

    if(!streams.size()) return;

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

    spdlog::debug("PlayerResponse : Deciphered URL [{}]", out);

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
    for (auto [source, ASUBIT] : this->_package) {
        if (ASUBIT.size()) return { source, ASUBIT };
    }

    throw std::logic_error("No audio stream source found !");
}

std::string AudioTube::StreamsManifest::preferedUrl() const {
    auto source = this->preferedStreamSource();
    spdlog::debug("Picking stream URL from source : [{}]", AudioStreamsSource_str[source.first]);

    // since bitrates are asc-ordered, take latest for fastest
    auto last = source.second.end();
    last--;

    return last->second.second;
}

bool AudioTube::StreamsManifest::isExpired() const {
    if (this->_validUntil == -1) return true;

    auto now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );

    return now > this->_validUntil;
}


bool AudioTube::StreamsManifest::_isCodecAllowed(const std::string &codec) {
    auto opusFound = codec.find("opus");
    return !(opusFound == std::string::npos);
}

bool AudioTube::StreamsManifest::_isMimeAllowed(const std::string &mime) {
    auto audioFound = mime.find("audio");
    if (audioFound == std::string::npos) return false;
    return _isCodecAllowed(mime);
}
