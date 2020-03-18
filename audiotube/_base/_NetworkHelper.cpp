#include "_NetworkHelper.h"

promise::Defer NetworkHelper::download(const QUrl& url, bool head) {

    return promise::newPromise([=](promise::Defer d) {

        QNetworkRequest request(url);
        auto manager = new QNetworkAccessManager;
        auto reply = head ? manager->head(request) : manager->get(request);  

        _pending++;

        //on finished
        QObject::connect(reply, &QNetworkReply::finished, [=]() {
            
            auto error = reply->error();

            if (error == QNetworkReply::NoError) {
                auto result = QString::fromUtf8(reply->readAll());
                d.resolve(static_cast<DownloadedUtf8>(result));
            } else {
                qWarning() << qUtf8Printable(QString("AudioTube : Error downloading [%1] : %2!")
                    .arg(url.toString())
                    .arg(error));
                d.reject(error);       
            }

            manager->deleteLater();
            reply->deleteLater();

            _pending--;
        });

    });
}