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

std::regex AudioTube::Regexes::Decipherer_findJSDecipheringOperations(const std::string &obfuscatedDecipheringFunctionName) {
    return std::regex { _Decipherer_generateRegex(_Decipherer_JSDecipheringOperations, obfuscatedDecipheringFunctionName) };
}

// Careful, order is important !
std::map<AudioTube::CipherOperation, std::regex> AudioTube::Regexes::Decipherer_DecipheringOps(const std::string &obfuscatedDecipheringFunctionName) {
    std::map<CipherOperation, std::regex> out;
    for (auto [co, regStrBase] : _cipherOperationRegexBase) {
        auto regexAsStr = _Decipherer_generateRegex(regStrBase, obfuscatedDecipheringFunctionName);

        out.emplace(
            co,
            std::regex { regexAsStr }
        );
    }
    return out;
}

std::string AudioTube::Regexes::_Decipherer_generateRegex(std::string rawRegexAsString, const std::string &obfuscatedDecipheringFunctionName) {
    std::string toReplace = "%1";
    auto escaped = escapeForRegex(obfuscatedDecipheringFunctionName);

    // replace
    rawRegexAsString.replace(
        rawRegexAsString.find(toReplace),
        toReplace.length(),
        escaped
    );

    // as std::string
    return std::string { rawRegexAsString };
}

std::string AudioTube::Regexes::escapeForRegex(const std::string &input) {
    // matches any characters that need to be escaped in RegEx
    return std::regex_replace(input, _specialChars, R"(\$&)");
}
