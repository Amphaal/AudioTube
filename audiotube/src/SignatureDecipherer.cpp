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

#include "SignatureDecipherer.h"

QString AudioTube::SignatureDecipherer::decipher(const QString &signature) const {
    auto modifiedSignature = signature;
    auto copyOfOperations = this->_operations;

    while (!copyOfOperations.isEmpty()) {
        switch (auto operationPair = copyOfOperations.dequeue(); operationPair.first) {
            case CipherOperation::Reverse: {
                std::reverse(modifiedSignature.begin(), modifiedSignature.end());
            }
            break;

            case CipherOperation::Slice: {
                auto targetIndex = operationPair.second.toInt();
                modifiedSignature = modifiedSignature.mid(targetIndex);
            }
            break;

            case CipherOperation::Swap: {
                auto firstIndex = 0;
                auto secondIndex = operationPair.second.toInt();

                auto first = QString(modifiedSignature.at(firstIndex));
                auto second = QString(modifiedSignature.at(secondIndex));

                modifiedSignature.replace(firstIndex, 1, second);
                modifiedSignature.replace(secondIndex, 1, first);
            }
            break;

            default:
                break;
        }
    }

    return modifiedSignature;
}

AudioTube::SignatureDecipherer* AudioTube::SignatureDecipherer::create(const QString &clientPlayerUrl, const QString &ytPlayerSourceCode) {
    auto newDecipher = new SignatureDecipherer(ytPlayerSourceCode);

     _cache.insert(clientPlayerUrl, newDecipher);

    return newDecipher;
}

AudioTube::SignatureDecipherer* AudioTube::SignatureDecipherer::fromCache(const QString &clientPlayerUrl) {
    return _cache.value(clientPlayerUrl);
}

AudioTube::SignatureDecipherer::YTClientMethod AudioTube::SignatureDecipherer::_findObfuscatedDecipheringFunctionName(const QString &ytPlayerSourceCode) {
    auto match = Regexes::Decipherer_findFunctionName.match(ytPlayerSourceCode);
    auto functionName = match.captured("functionName");

    if (functionName.isEmpty()) {
        throw std::runtime_error("[Decipherer] No function name found !");
    }

    return functionName;
}

QList<QString> AudioTube::SignatureDecipherer::_findJSDecipheringOperations(const QString &ytPlayerSourceCode, const YTClientMethod &obfuscatedDecipheringFunctionName) {
    // get the body of the function
    auto regex = Regexes::Decipherer_findJSDecipheringOperations(obfuscatedDecipheringFunctionName);
    auto match = regex.match(ytPlayerSourceCode);

    auto functionBody = match.captured("functionBody");
    if (functionBody.isEmpty()) {
        throw std::runtime_error("[Decipherer] No function body found !");
    }

    // calls
    auto javascriptFunctionCalls = functionBody.split(";", Qt::SkipEmptyParts);

    return std::move(javascriptFunctionCalls);
}

QHash<AudioTube::CipherOperation, AudioTube::SignatureDecipherer::YTClientMethod> AudioTube::SignatureDecipherer::
    _findObfuscatedDecipheringOperationsFunctionName(const QString &ytPlayerSourceCode, const QList<QString> &javascriptDecipheringOperations) {
    // define out
    QHash<CipherOperation, YTClientMethod> functionNamesByOperation;

    // find subjacent functions names used by decipherer
    QSet<QString> uniqueOperations;
    for (const auto &call : javascriptDecipheringOperations) {
        // find function name in method call
        auto match = Regexes::Decipherer_findCalledFunction.match(call);
        if (!match.hasMatch()) continue;
        auto calledFunctionName = match.captured("functionName");

        // add to set
        uniqueOperations.insert(calledFunctionName);
    }

    // iterate through unique operations
    for (const auto &calledFunctionName : uniqueOperations) {
        // custom regexes to determine associated decipherer method
        auto customRegexes = Regexes::Decipherer_DecipheringOps(calledFunctionName);

        // find...
        for (auto i = customRegexes.begin(); i != customRegexes.end(); ++i) {
            // if already found, skip
            auto co = i.key();
            if (functionNamesByOperation.contains(co)) continue;

            // check with regex
            auto regex = i.value();
            if (regex.match(ytPlayerSourceCode).hasMatch()) {
                functionNamesByOperation.insert(co, calledFunctionName);
                break;
            }
        }
    }

    if (!functionNamesByOperation.count()) {
        throw std::runtime_error("[Decipherer] Missing function names by operations !");
    }

    return functionNamesByOperation;
}

AudioTube::SignatureDecipherer::YTDecipheringOperations AudioTube::SignatureDecipherer::_buildOperations(
        const QHash<CipherOperation, YTClientMethod> &functionNamesByOperation,
        const QList<QString> &javascriptOperations
    ) {
    YTDecipheringOperations operations;

    // determine order and execution of subjacent methods

    // iterate
    for (const auto &call : javascriptOperations) {
        // find which function is called
        auto match = Regexes::Decipherer_findFuncAndArgument.match(call);
        if (!match.hasMatch()) continue;

        auto calledFunctionName = match.captured("functionName");
        auto arg = match.captured("arg").toInt();

        // by operation type
        switch (auto operationType = functionNamesByOperation.key(calledFunctionName)) {
            case CipherOperation::Reverse: {
                operations.enqueue({operationType, QVariant()});
            }
            break;

            case CipherOperation::Slice:
            case CipherOperation::Swap: {
                operations.enqueue({operationType, QVariant(arg)});
            }
            break;

            default:
                break;
        }
    }

    if (!operations.count()) {
        throw std::runtime_error("[Decipherer] No operations found !");
    }

    return operations;
}

AudioTube::SignatureDecipherer::SignatureDecipherer(const QString &ytPlayerSourceCode) {
    // find deciphering function name
    auto functionName = _findObfuscatedDecipheringFunctionName(ytPlayerSourceCode);

    // get JS deciphering operations
    auto javascriptOperations = _findJSDecipheringOperations(ytPlayerSourceCode, functionName);

    // find operations functions name
    auto functionNamesByOperation = _findObfuscatedDecipheringOperationsFunctionName(ytPlayerSourceCode, javascriptOperations);

    // generate operations
    auto operations = _buildOperations(functionNamesByOperation, javascriptOperations);

    // copy operation to object
    this->_operations = operations;
}
