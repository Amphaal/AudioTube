#include "_NetworkHelper.h"

promise::Defer NetworkHelper::download(const QUrl& url, bool head) {

    if(!_manager) {
        _manager = new QNetworkAccessManager;
    }

    return promise::newPromise([=](promise::Defer d) {

        QNetworkRequest request(url);
        auto reply = head ? _manager->head(request) : _manager->get(request);  

        //use local event loop to mimic signal/slots same-thread async behavior
        QEventLoop loop;
        QObject::connect(
            reply, &QNetworkReply::finished, 
            &loop, &QEventLoop::quit
        );
        loop.exec();

        //if no error...    
        if (auto error = reply->error(); !error) {

            auto result = static_cast<DownloadedUtf8>(QString::fromUtf8(reply->readAll()));
            
            delete reply;
            d.resolve(result);

        }

        //if error
        else {

            qWarning() << qUtf8Printable(QString("AudioTube : Error downloading [%1] : %2!")
                .arg(url.toString())
                .arg(error));

            delete reply;
            d.reject(error);    

        }

    });
}