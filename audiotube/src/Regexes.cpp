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

std::string AudioTube::Regexes::Decipherer_findJSDecipheringOperations(const std::string &obfuscatedDecipheringFunctionName) {
    return _Decipherer_generateRegex(_Decipherer_JSDecipheringOperations, obfuscatedDecipheringFunctionName);
}

// Careful, order is important !
std::map<AudioTube::CipherOperation, std::string> AudioTube::Regexes::Decipherer_DecipheringOps(const std::string &obfuscatedDecipheringFunctionName) {
    std::map<CipherOperation, std::string> out;
    for (auto i = _cipherOperationRegexBase.begin(); i != _cipherOperationRegexBase.end(); ++i) {
        auto regex = _Decipherer_generateRegex(i->second, obfuscatedDecipheringFunctionName);
        out.emplace(i->first, regex);
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
