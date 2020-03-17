#pragma once

#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QFile>

class DebugHelper {
    public:
        static void _dumpAsJSON(const QUrlQuery &query);
        static void _dumpAsJSON(const QJsonObject &obj);
        static void _dumpAsJSON(const QJsonArray &arr);
        static void _dumpAsJSON(const QJsonDocument &doc);
};