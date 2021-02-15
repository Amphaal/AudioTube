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

#define PCRE2_STATIC
#include "jpcre2.hpp"

#include "CipherOperation.h"

typedef jpcre2::select<char> jp;

namespace AudioTube {

class Regexes {
 public:
    // 1# <itag>, 2# <codec>, 3# <bitrate>, 4# <url>
    static inline jp::Regex DASHManifestExtractor {
        R"|(<Representation id="(.*?)" codecs="(.*?)"[\s\S]*?bandwidth="(.*?)"[\s\S]*?<BaseURL>(.*?)<\/BaseURL)|"
    };

    // #1 <videoId>
    static inline jp::Regex YoutubeIdFinder {
        R"|((?:youtube\.com|youtu.be).*?(?:v=|embed\/)([\w\-]+))|"
    };

    // #1 <videoId>
    static inline jp::Regex HTTPRequestYTVideoIdExtractor {
        R"|(watch\?v=(.*?)&amp;)|"
    };

    // https://stackoverflow.com/questions/546433/regular-expression-to-match-balanced-parentheses
    static inline jp::Regex BalancedBraces {
        R"|(\{(?:[^}{]*(?R)?)*+\})|"
    };

    static inline jp::Regex FindScriptSrc {
        R"|(<script.*?src="(.*?)".*?>)|"
    };

    static inline jp::Regex PlayerConfigExtractorFromEmbed_JSONStart {
        R"|(['""]PLAYER_CONFIG['""]\s*:\s*(.*))|"
    };

    // #1 <playerConfig>
    static inline jp::Regex PlayerConfigExtractorFromWatchPage_JSONStart {
        R"|(var\s+ytInitialPlayerResponse\s*=\s*(\{.*\}))|"
    };

    // #1 <sts>
    static inline jp::Regex STSFinder {
        R"|((?<=signatureTimestamp[=\:])\d+)|"
    };

    // #1 <functionName>, #2 <arg>
    static inline jp::Regex Decipherer_findFuncAndArgument {
        R"|(\.(\w+)\(\w+,(\d+)\))|"
    };

    // #1 <functionName>
    static inline jp::Regex Decipherer_findFunctionName {
        R"|((\w+)=function\(\w+\)\{(\w+)=\2\.split\(\x22{2}\);.*?return\s+\2\.join\(\x22{2}\)\})|"
    };

    // #1 <functionName>
    static inline jp::Regex Decipherer_findCalledFunction {
        R"|(\w+\.(\w+)\()|"
    };

    // Careful, order is important !
    static std::map<CipherOperation, jp::Regex> Decipherer_DecipheringOps(const std::string &obfuscatedDecipheringFunctionName);
    static jp::Regex Decipherer_findJSDecipheringOperations(const std::string &obfuscatedDecipheringFunctionName);
    static std::string escapeForRegex(const std::string &toEscapeForRegex);

 private:
    static inline std::regex _specialChars { R"([-[\]{}()*+?.,\^$|#\s])" };

    // #1 <functionBody>
    static constexpr auto _Decipherer_JSDecipheringOperations {
        R"|(%1=function\(\w+\)\{(.*?)\})|"
    };

    // Careful, order is important !
    static inline std::map<CipherOperation, std::string> _cipherOperationRegexBase {
        { CipherOperation::Slice, R"|(%1:\bfunction\b\([a],b\).(\breturn\b)?.?\w+\.)|" },
        { CipherOperation::Swap, R"|(%1:\bfunction\b\(\w+\,\w\).\bvar\b.\bc=a\b)|" },
        { CipherOperation::Reverse, R"|(%1:\bfunction\b\(\w+\))|" }
    };

    static std::string _Decipherer_generateRegex(std::string rawRegexAsString, const std::string &obfuscatedDecipheringFunctionName);
};

}  // namespace AudioTube
