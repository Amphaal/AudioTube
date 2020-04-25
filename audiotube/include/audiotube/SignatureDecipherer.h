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

#include <QHash>
#include <QString>

#include <QPair>
#include <QQueue>
#include <QVariant>

#include <QDebug>

#include "Regexes.h"
#include "CipherOperation.h"
#include "_DebugHelper.h"

namespace AudioTube {

class SignatureDecipherer {
 public:
    QString decipher(const QString &signature) const;
    static SignatureDecipherer* create(const QString &clientPlayerUrl, const QString &rawPlayerSourceData);
    static SignatureDecipherer* fromCache(const QString &clientPlayerUrl);

 private:
    using YTDecipheringOperations = QQueue<QPair<CipherOperation, QVariant>>;
    using YTClientMethod = QString;

    explicit SignatureDecipherer(const QString &rawPlayerSourceData);

    QQueue<QPair<CipherOperation, QVariant>> _operations;
    static inline QHash<QString, SignatureDecipherer*> _cache;

    static YTClientMethod _findObfuscatedDecipheringFunctionName(const QString &ytPlayerSourceCode);
    static QList<QString>
        _findJSDecipheringOperations(const QString &ytPlayerSourceCode, const YTClientMethod &obfuscatedDecipheringFunctionName);
    static QHash<CipherOperation, YTClientMethod>
        _findObfuscatedDecipheringOperationsFunctionName(const QString &ytPlayerSourceCode, const QList<QString> &javascriptDecipheringOperations);
    static YTDecipheringOperations
        _buildOperations(const QHash<CipherOperation, YTClientMethod> &functionNamesByOperation, const QList<QString> &javascriptOperations);
};

}  // namespace AudioTube
