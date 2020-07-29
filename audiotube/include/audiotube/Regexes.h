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

#pragma once

#include <regex>

#include "CipherOperation.h"

namespace AudioTube {

class Regexes {
 public:
    static inline std::regex DASHManifestExtractor = std::regex(
        R"|(<Representation id="(?<itag>.*?)" codecs="(?<codec>.*?)"[\s\S]*?bandwidth="(?<bitrate>.*?)"[\s\S]*?<BaseURL>(?<url>.*?)<\/BaseURL)|"
    );
    static inline std::regex YoutubeIdFinder = std::regex(
        R"|((?:youtube\.com|youtu.be).*?(?:v=|embed\/)(?<videoId>[\w\-]+))|"
    );
    static inline std::regex HTTPRequestYTVideoIdExtractor = std::regex(
        R"|(watch\?v=(?<videoId>.*?)&amp;)|"
    );
    static inline std::regex PlayerConfigExtractorFromEmbed = std::regex(
        R"|(yt\.setConfig\({'PLAYER_CONFIG': (?<playerConfig>.*?)}\);)|"
    );
    static inline std::regex PlayerConfigExtractorFromWatchPage = std::regex(
        R"|(ytplayer\.config = (?<playerConfig>.*?)\;ytplayer)|"
    );
    static inline std::regex STSFinder = std::regex(
        R"|(invalid namespace.*?;.*?=(?<sts>\d+);)|"
    );

    static inline std::regex Decipherer_findFuncAndArgument = std::regex(
        R"|(\.(?<functionName>\w+)\(\w+,(?<arg>\d+)\))|"
    );
    static inline std::regex Decipherer_findFunctionName = std::regex(
        R"|((?<functionName>\w+)=function\(\w+\){(\w+)=\2\.split\(\x22{2}\);.*?return\s+\2\.join\(\x22{2}\)})|"
    );
    static inline std::regex Decipherer_findCalledFunction = std::regex(
        R"|(\w+\.(?<functionName>\w+)\()|"
    );
    static std::regex Decipherer_findJSDecipheringOperations(const std::string &obfuscatedDecipheringFunctionName) {
        return _Decipherer_generateRegex(_Decipherer_JSDecipheringOperations, obfuscatedDecipheringFunctionName);
    }

    // Careful, order is important !
    static QMap<CipherOperation, std::regex> Decipherer_DecipheringOps(const std::string &obfuscatedDecipheringFunctionName) {
        QMap<CipherOperation, std::regex> out;
        for (auto i = _cipherOperationRegexBase.begin(); i != _cipherOperationRegexBase.end(); ++i) {
            auto regex = _Decipherer_generateRegex(i.value(), obfuscatedDecipheringFunctionName);
            out.insert(i.key(), regex);
        }
        return out;
    }

 private:
    static std::regex _Decipherer_generateRegex(const std::string &regex, const std::string &obfuscatedDecipheringFunctionName) {
        auto escaped = std::regex::escape(obfuscatedDecipheringFunctionName);
        return std::regex(
            regex.arg(escaped)
        );
    }
    static inline std::string _Decipherer_JSDecipheringOperations = std::string(
        R"|((?!h\.)%1=function\(\w+\)\{(?<functionBody>.*?)\})|"
    );

    // Careful, order is important !
    static inline QMap<CipherOperation, std::string> _cipherOperationRegexBase {
        { CipherOperation::Slice, R"|(%1:\bfunction\b\([a],b\).(\breturn\b)?.?\w+\.)|" },
        { CipherOperation::Swap, R"|(%1:\bfunction\b\(\w+\,\w\).\bvar\b.\bc=a\b)|" },
        { CipherOperation::Reverse, R"|(%1:\bfunction\b\(\w+\))|" }
    };
};

}  // namespace AudioTube
