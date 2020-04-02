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
#include <QRegularExpression>
#include <QPair>
#include <QQueue>
#include <QVariant>

#include <QDebug>

class SignatureDecipherer {
    
    public:
        enum class CipherOperation { CO_Unknown, Reverse, Slice, Swap };

        QString decipher(const QString &signature) const;
        static SignatureDecipherer* create(const QString &clientPlayerUrl, const QString &rawPlayerSourceData);
        static SignatureDecipherer* fromCache(const QString &clientPlayerUrl);

    private:
        using YTDecipheringOperations = QQueue<QPair<SignatureDecipherer::CipherOperation, QVariant>>;
        using YTClientMethod = QString;

        SignatureDecipherer(const QString &rawPlayerSourceData);
        
        QQueue<QPair<SignatureDecipherer::CipherOperation, QVariant>> _operations;
        static inline QHash<QString, SignatureDecipherer*> _cache;

        static YTClientMethod _findObfuscatedDecipheringFunctionName(const QString &ytPlayerSourceCode);
        static QList<QString> _findJSDecipheringOperations(const QString &ytPlayerSourceCode, const YTClientMethod &obfuscatedDecipheringFunctionName);
        static QHash<SignatureDecipherer::CipherOperation, YTClientMethod> _findObfuscatedDecipheringOperationsFunctionName(const QString &ytPlayerSourceCode, QList<QString> &javascriptDecipheringOperations);
        static YTDecipheringOperations _buildOperations(QHash<SignatureDecipherer::CipherOperation, YTClientMethod> &functionNamesByOperation, 
                                                        QList<QString> &javascriptOperations);
};
