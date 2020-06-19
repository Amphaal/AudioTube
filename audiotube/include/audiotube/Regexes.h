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

#include <QRegularExpression>

#include "CipherOperation.h"

namespace AudioTube {

class Regexes {
 public:
    static inline QRegularExpression DASHManifestExtractor = QRegularExpression(
        R"|(<Representation id="(?<itag>.*?)" codecs="(?<codec>.*?)"[\s\S]*?bandwidth="(?<bitrate>.*?)"[\s\S]*?<BaseURL>(?<url>.*?)<\/BaseURL)|"
    );
    static inline QRegularExpression YoutubeIdFinder = QRegularExpression(
        R"|((?:youtube\.com|youtu.be).*?(?:v=|embed\/)(?<videoId>[\w\-]+))|"
    );
    static inline QRegularExpression HTTPRequestYTVideoIdExtractor = QRegularExpression(
        R"|(watch\?v=(?<videoId>.*?)&amp;)|"
    );
    static inline QRegularExpression PlayerConfigExtractorFromEmbed = QRegularExpression(
        R"|(yt\.setConfig\({'PLAYER_CONFIG': (?<playerConfig>.*?)}\);)|"
    );
    static inline QRegularExpression PlayerConfigExtractorFromWatchPage = QRegularExpression(
        R"|(ytplayer\.config = (?<playerConfig>.*?)\;ytplayer)|"
    );
    static inline QRegularExpression STSFinder = QRegularExpression(
        R"|(invalid namespace.*?;.*?=(?<sts>\d+);)|"
    );

    static inline QRegularExpression Decipherer_findFuncAndArgument = QRegularExpression(
        R"|(\.(?<functionName>\w+)\(\w+,(?<arg>\d+)\))|"
    );
    static inline QRegularExpression Decipherer_findFunctionName = QRegularExpression(
        R"|((?<functionName>\w+)=function\(\w+\){(\w+)=\2\.split\(\x22{2}\);.*?return\s+\2\.join\(\x22{2}\)})|"
    );
    static inline QRegularExpression Decipherer_findCalledFunction = QRegularExpression(
        R"|(\w+\.(?<functionName>\w+)\()|"
    );
    static QRegularExpression Decipherer_findJSDecipheringOperations(const QString &obfuscatedDecipheringFunctionName) {
        return _Decipherer_generateRegex(_Decipherer_JSDecipheringOperations, obfuscatedDecipheringFunctionName);
    }

    // Careful, order is important !
    static QMap<CipherOperation, QRegularExpression> Decipherer_DecipheringOps(const QString &obfuscatedDecipheringFunctionName) {
        QMap<CipherOperation, QRegularExpression> out;
        for (auto i = _cipherOperationRegexBase.begin(); i != _cipherOperationRegexBase.end(); ++i) {
            auto regex = _Decipherer_generateRegex(i.value(), obfuscatedDecipheringFunctionName);
            out.insert(i.key(), regex);
        }
        return out;
    }

 private:
    static QRegularExpression _Decipherer_generateRegex(const QString &regex, const QString &obfuscatedDecipheringFunctionName) {
        auto escaped = QRegularExpression::escape(obfuscatedDecipheringFunctionName);
        return QRegularExpression(
            regex.arg(escaped)
        );
    }
    static inline QString _Decipherer_JSDecipheringOperations = QString(
        R"|((?!h\.)%1=function\(\w+\)\{(?<functionBody>.*?)\})|"
    );

    // Careful, order is important !
    static inline QMap<CipherOperation, QString> _cipherOperationRegexBase {
        { CipherOperation::Slice, R"|(%1:\bfunction\b\([a],b\).(\breturn\b)?.?\w+\.)|" },
        { CipherOperation::Swap, R"|(%1:\bfunction\b\(\w+\,\w\).\bvar\b.\bc=a\b)|" },
        { CipherOperation::Reverse, R"|(%1:\bfunction\b\(\w+\))|" }
    };
};

}  // namespace AudioTube
