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

#include <string>
#include <map>
#include <regex>

#include "CipherOperation.h"

namespace AudioTube {

class Regexes {
 public:
    // 1# <itag>, 2# <codec>, 3# <bitrate>, 4# <url>
    static inline std::regex DASHManifestExtractor {
        R"|(<Representation id="(.*?)" codecs="(.*?)"[\s\S]*?bandwidth="(.*?)"[\s\S]*?<BaseURL>(.*?)<\/BaseURL)|"
    };

    // #1 <videoId>
    static inline std::regex YoutubeIdFinder {
        R"|((?:youtube\.com|youtu.be).*?(?:v=|embed\/)([\w\-]+))|"
    };

    // #1 <videoId>
    static inline std::regex HTTPRequestYTVideoIdExtractor {
        R"|(watch\?v=(.*?)&amp;)|"
    };

    // #1 <playerConfig>
    static inline std::regex PlayerConfigExtractorFromEmbed {
        R"|(yt\.setConfig\(\{'PLAYER_CONFIG': (.*?)\}\);)|"
    };

    // #1 <playerConfig>
    static inline std::regex PlayerConfigExtractorFromWatchPage {
        R"|(ytplayer\.config = (.*?)\;ytplayer)|"
    };

    // #1 <sts>
    static inline std::regex STSFinder {
        R"|(invalid namespace.*?;.*?=(\d+);)|"
    };

    // #1 <functionName>, #2 <arg>
    static inline std::regex Decipherer_findFuncAndArgument {
        R"|(\.(\w+)\(\w+,(\d+)\))|"
    };

    // #1 <functionName>
    static inline std::regex Decipherer_findFunctionName {
        R"|((\w+)=function\(\w+\)\{(\w+)=\2\.split\(\x22{2}\);.*?return\s+\2\.join\(\x22{2}\)\})|"
    };

    // #1 <functionName>
    static inline std::regex Decipherer_findCalledFunction {
        R"|(\w+\.(\w+)\()|"
    };

    // Careful, order is important !
    static std::map<CipherOperation, std::regex> Decipherer_DecipheringOps(const std::string &obfuscatedDecipheringFunctionName);
    static std::regex Decipherer_findJSDecipheringOperations(const std::string &obfuscatedDecipheringFunctionName);

 private:
    static inline std::regex _specialChars { R"|([-[\]{}()*+?.,\^$|#\s])|" };

    // #1 <functionBody>
    static constexpr auto _Decipherer_JSDecipheringOperations {
        R"|((?!h\.)%1=function\(\w+\)\{(.*?)\})|"
    };

    // Careful, order is important !
    static inline std::map<CipherOperation, std::string> _cipherOperationRegexBase {
        { CipherOperation::Slice, R"|(%1:\bfunction\b\([a],b\).(\breturn\b)?.?\w+\.)|" },
        { CipherOperation::Swap, R"|(%1:\bfunction\b\(\w+\,\w\).\bvar\b.\bc=a\b)|" },
        { CipherOperation::Reverse, R"|(%1:\bfunction\b\(\w+\))|" }
    };

    static std::string _Decipherer_generateRegex(std::string rawRegexAsString, const std::string &obfuscatedDecipheringFunctionName);
    static std::string escapeForRegex(const std::string &input);
};

}  // namespace AudioTube
