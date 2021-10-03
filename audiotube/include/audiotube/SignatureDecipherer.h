// AudioTube C++
// C++ fork based on https://github.com/Tyrrrz/YoutubeExplode
// Copyright (C) 2019-2021 Guillaume Vara

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "Regexes.h"
#include "CipherOperation.h"
#include "_DebugHelper.h"
#include "ATHelper.h"

#include <queue>
#include <utility>
#include <vector>
#include <set>
#include <unordered_map>
#include <string>

namespace AudioTube {

class SignatureDecipherer {
 public:
    using Argument = int;
    using YTDecipheringOperations = std::queue<std::pair<CipherOperation, Argument>>;
    using YTClientMethod = std::string;

    void printOperations() const;
    std::string decipher(const std::string &signature) const;

    static SignatureDecipherer* create(const std::string &clientPlayerUrl, const std::string &rawPlayerSourceData);
    static SignatureDecipherer* fromCache(const std::string &clientPlayerUrl);

    explicit SignatureDecipherer(const YTDecipheringOperations &operations);

 private:
    explicit SignatureDecipherer(const std::string &rawPlayerSourceData);

    YTDecipheringOperations _operations;
    static inline std::unordered_map<std::string, SignatureDecipherer*> _cache;

    static YTClientMethod _findObfuscatedDecipheringFunctionName(const std::string &ytPlayerSourceCode);
    static std::vector<std::string>
        _findJSDecipheringOperations(const std::string &ytPlayerSourceCode, const YTClientMethod &obfuscatedDecipheringFunctionName);
    static std::unordered_map<CipherOperation, YTClientMethod>
        _findObfuscatedDecipheringOperationsFunctionName(const std::string &ytPlayerSourceCode, const std::vector<std::string> &javascriptDecipheringOperations);
    static YTDecipheringOperations
        _buildOperations(const std::unordered_map<CipherOperation, YTClientMethod> &functionNamesByOperation, const std::vector<std::string> &javascriptOperations);
};

}  // namespace AudioTube
