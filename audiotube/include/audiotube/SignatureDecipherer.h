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

#include "Regexes.h"
#include "CipherOperation.h"
#include "_DebugHelper.h"

namespace AudioTube {

class SignatureDecipherer {
 public:
    std::string decipher(const std::string &signature) const;
    static SignatureDecipherer* create(const std::string &clientPlayerUrl, const std::string &rawPlayerSourceData);
    static SignatureDecipherer* fromCache(const std::string &clientPlayerUrl);

 private:
    using YTDecipheringOperations = std::deque<std::pair<CipherOperation, QVariant>>;
    using YTClientMethod = std::string;

    explicit SignatureDecipherer(const std::string &rawPlayerSourceData);

    std::deque<std::pair<CipherOperation, QVariant>> _operations;
    static inline QHash<std::string, SignatureDecipherer*> _cache;

    static YTClientMethod _findObfuscatedDecipheringFunctionName(const std::string &ytPlayerSourceCode);
    static QList<std::string>
        _findJSDecipheringOperations(const std::string &ytPlayerSourceCode, const YTClientMethod &obfuscatedDecipheringFunctionName);
    static QHash<CipherOperation, YTClientMethod>
        _findObfuscatedDecipheringOperationsFunctionName(const std::string &ytPlayerSourceCode, const QList<std::string> &javascriptDecipheringOperations);
    static YTDecipheringOperations
        _buildOperations(const QHash<CipherOperation, YTClientMethod> &functionNamesByOperation, const QList<std::string> &javascriptOperations);
};

}  // namespace AudioTube
