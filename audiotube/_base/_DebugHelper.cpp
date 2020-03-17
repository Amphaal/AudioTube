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

void DebugHelper::_dumpAsJSON(const QJsonDocument &doc) {
    
    auto bytes = doc.toJson(QJsonDocument::JsonFormat::Indented);

    QFile fh("yt.json");
    fh.open(QFile::WriteOnly);
        fh.write(bytes);
    fh.close();

    qDebug() << fh.fileName() << " created !";

}