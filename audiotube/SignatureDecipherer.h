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

class SignatureDecipherer {
    
    public:
        enum class CipherOperation { CO_Unknown, Reverse, Slice, Swap };

        QString decipher(const QString &signature) const;
        static SignatureDecipherer* create(const QString &clientPlayerUrl, const QString &rawPlayerSourceData);
        static SignatureDecipherer* fromCache(const QString &clientPlayerUrl);

    private:
        using YTDecipheringOperations = QQueue<QPair<SignatureDecipherer::CipherOperation, QVariant>>;
        using YTClientMethod = QString;

        SignatureDecipherer(const QString &rawPlayerSourceData);
        
        QQueue<QPair<SignatureDecipherer::CipherOperation, QVariant>> _operations;
        static inline QHash<QString, SignatureDecipherer*> _cache;

        static YTClientMethod _findObfuscatedDecipheringFunctionName(const QString &ytPlayerSourceCode);
        static QList<QString> _findJSDecipheringOperations(const QString &ytPlayerSourceCode, const YTClientMethod &obfuscatedDecipheringFunctionName);
        static QHash<SignatureDecipherer::CipherOperation, YTClientMethod> _findObfuscatedDecipheringOperationsFunctionName(const QString &ytPlayerSourceCode, QList<QString> &javascriptDecipheringOperations);
        static YTDecipheringOperations _buildOperations(QHash<SignatureDecipherer::CipherOperation, YTClientMethod> &functionNamesByOperation, 
                                                        QList<QString> &javascriptOperations);
};
