////////////////////////////////////////////////////////////////////////////
// BASED ON THE WORK OF Alexey "Tyrrrz" Golub (https://github.com/Tyrrrz) //
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QHash>
#include <QString>
#include <QRegularExpression>
#include <QPair>
#include <QQueue>
#include <QVariant>

#include <QDebug>

class YoutubeSignatureDecipherer {
    
    public:
        enum class CipherOperation { CO_Unknown, Reverse, Slice, Swap };

        QString decipher(const QString &signature);
        static YoutubeSignatureDecipherer* create(const QString &clientPlayerUrl, const QString &rawPlayerSourceData);
        static YoutubeSignatureDecipherer* fromCache(const QString &clientPlayerUrl);

    private:
        using YTDecipheringOperations = QQueue<QPair<YoutubeSignatureDecipherer::CipherOperation, QVariant>>;
        using YTClientMethod = QString;

        YoutubeSignatureDecipherer(const QString &rawPlayerSourceData);
        
        QQueue<QPair<YoutubeSignatureDecipherer::CipherOperation, QVariant>> _operations;
        static inline QHash<QString, YoutubeSignatureDecipherer*> _cache;

        static YTClientMethod _findObfuscatedDecipheringFunctionName(const QString &ytPlayerSourceCode);
        static QList<QString> _findJSDecipheringOperations(const QString &ytPlayerSourceCode, const YTClientMethod &obfuscatedDecipheringFunctionName);
        static QHash<YoutubeSignatureDecipherer::CipherOperation, YTClientMethod> _findObfuscatedDecipheringOperationsFunctionName(const QString &ytPlayerSourceCode, QList<QString> &javascriptDecipheringOperations);
        static YTDecipheringOperations _buildOperations(QHash<YoutubeSignatureDecipherer::CipherOperation, YTClientMethod> &functionNamesByOperation, 
                                                        QList<QString> &javascriptOperations);
};
