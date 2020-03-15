#include "_NetworkHelper.h"

promise::Defer NetworkHelper::download(const QUrl& url) {

    return promise::newPromise([=](promise::Defer d) {

        QNetworkRequest request(url);
        auto manager = new QNetworkAccessManager;
        auto reply = manager->get(request);  

        _pending++;

        //on finished
        QObject::connect(reply, &QNetworkReply::finished, [=]() {
            
            auto error = reply->error();

            if (error == QNetworkReply::NoError) {
                auto result = reply->readAll();
                d.resolve(result);
            } else {
                d.reject(error);       
            }

            manager->deleteLater();
            reply->deleteLater();

            _pending--;
        });

    });
}