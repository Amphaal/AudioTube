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

#include "Regexes.h"

pcre2cppRE AudioTube::Regexes::Decipherer_findJSDecipheringOperations(const std::string &obfuscatedDecipheringFunctionName) {
    return pcre2cppRE { _Decipherer_generateRegex(_Decipherer_JSDecipheringOperations, obfuscatedDecipheringFunctionName) };
}

// Careful, order is important !
std::map<AudioTube::CipherOperation, pcre2cppRE> AudioTube::Regexes::Decipherer_DecipheringOps(const std::string &obfuscatedDecipheringFunctionName) {
    std::map<CipherOperation, pcre2cppRE> out;
    for (auto [co, regStrBase] : _cipherOperationRegexBase) {
        auto regexAsStr = _Decipherer_generateRegex(regStrBase, obfuscatedDecipheringFunctionName);

        out.emplace(
            co,
            pcre2cppRE { regexAsStr }
        );
    }
    return out;
}

std::string AudioTube::Regexes::_Decipherer_generateRegex(std::string rawRegexAsString, const std::string &obfuscatedDecipheringFunctionName) {
    std::string toReplace = "%1";
    auto escaped = pcre2cppRE::QuoteMeta(obfuscatedDecipheringFunctionName);

    // replace
    rawRegexAsString.replace(
        rawRegexAsString.find(toReplace),
        toReplace.length(),
        escaped
    );

    // as std::string
    return std::string { rawRegexAsString };
}
