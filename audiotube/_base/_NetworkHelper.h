#pragma once

#include <promise-cpp/promise.hpp>

#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QEventLoop>

class NetworkHelper {
    public:
        using DownloadedUtf8 = QString;

    protected:
        static promise::Defer download(const QUrl& url, bool head = false);

    private:
        static inline QNetworkAccessManager* _manager = nullptr;
};