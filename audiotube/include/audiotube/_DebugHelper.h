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

#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QFile>

namespace AudioTube {

class DebugHelper {
 public:
    static void _dumpAsJSON(const QUrlQuery &query);
    static void _dumpAsJSON(const QJsonObject &obj);
    static void _dumpAsJSON(const QJsonArray &arr);
    static void _dumpAsJSON(const QJsonDocument &doc);
    static void _dumpAsFile(const QString &str);
};

}  // namespace AudioTube
