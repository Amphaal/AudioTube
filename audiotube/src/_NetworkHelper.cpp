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

#include "_NetworkHelper.h"

promise::Defer AudioTube::NetworkHelper::download(const std::string &url, bool head) {
    if (!_manager) {
        _manager = new QNetworkAccessManager;
    }

    return promise::newPromise([=](promise::Defer d) {
        QNetworkRequest request(url);
        auto reply = head ? _manager->head(request) : _manager->get(request);

        // use local event loop to mimic signal/slots same-thread async behavior
        QEventLoop loop;
        QObject::connect(
            reply, &QNetworkReply::finished,
            &loop, &QEventLoop::quit
        );
        loop.exec();

        // if no error...
        if (auto error = reply->error(); !error) {
            auto result = static_cast<DownloadedUtf8>(std::string::fromUtf8(reply->readAll()));

            delete reply;
            d.resolve(result);
        } else {  // if error
            spdlog::warn("AudioTube : Error downloading [{}] : {}!", url.toString(), error);

            delete reply;
            d.reject(error);
        }
    });
}
