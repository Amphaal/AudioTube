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

#include "_DebugHelper.h"

void DebugHelper::_dumpAsJSON(const QUrlQuery &query) {
    
    QJsonObject dump;
    
    for(const auto &item : query.queryItems(QUrl::ComponentFormattingOption::FullyDecoded)) {
        dump.insert(item.first, item.second);
    }

    _dumpAsJSON(dump);

}

void DebugHelper::_dumpAsJSON(const QJsonObject &obj) {
    return _dumpAsJSON(QJsonDocument(obj));
}

void DebugHelper::_dumpAsJSON(const QJsonArray &arr) {
    return _dumpAsJSON(QJsonDocument(arr));
}

void DebugHelper::_dumpAsFile(const QString &str) {

    QFile fh("yt.txt");
    fh.open(QFile::WriteOnly);
        fh.write(str.toUtf8());
    fh.close();

    qDebug() << fh.fileName() << " created !";

}

void DebugHelper::_dumpAsJSON(const QJsonDocument &doc) {
    
    auto bytes = doc.toJson(QJsonDocument::JsonFormat::Indented);

    QFile fh("yt.json");
    fh.open(QFile::WriteOnly);
        fh.write(bytes);
    fh.close();

    qDebug() << fh.fileName() << " created !";

}