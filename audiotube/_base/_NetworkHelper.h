#pragma once

#include <promise-cpp/promise.hpp>

#include <QNetworkReply>
#include <QNetworkAccessManager>

class NetworkHelper {
    protected:
        static promise::Defer download(const QUrl& url, bool head = false);

    private:
        static inline int _pending = 0;
};