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

std::string AudioTube::SignatureDecipherer::decipher(const std::string &signature) const {
    auto modifiedSignature = signature;
    auto copyOfOperations = this->_operations;

    while (!copyOfOperations.empty()) {
        auto operationPair = copyOfOperations.front();
        copyOfOperations.pop();

        switch (operationPair.first) {
            case CipherOperation::Reverse: {
                std::reverse(modifiedSignature.begin(), modifiedSignature.end());
            }
            break;

            case CipherOperation::Slice: {
                auto targetIndex = operationPair.second;
                modifiedSignature = modifiedSignature.substr(targetIndex);
            }
            break;

            case CipherOperation::Swap: {
                auto firstIndex = 0;
                auto secondIndex = operationPair.second;

                auto first = modifiedSignature[firstIndex];
                auto second = modifiedSignature[secondIndex];

                modifiedSignature.replace(firstIndex, 1, &second);
                modifiedSignature.replace(secondIndex, 1, &first);
            }
            break;

            default:
                break;
        }
    }

    return modifiedSignature;
}

AudioTube::SignatureDecipherer* AudioTube::SignatureDecipherer::create(const std::string &clientPlayerUrl, const std::string &ytPlayerSourceCode) {
    auto newDecipher = new SignatureDecipherer(ytPlayerSourceCode);

     _cache.emplace(clientPlayerUrl, newDecipher);

    return newDecipher;
}

AudioTube::SignatureDecipherer* AudioTube::SignatureDecipherer::fromCache(const std::string &clientPlayerUrl) {
    if (auto found = _cache.find(clientPlayerUrl); found != _cache.end()) {
        return found->second;
    }
    return nullptr;
}

AudioTube::SignatureDecipherer::YTClientMethod AudioTube::SignatureDecipherer::_findObfuscatedDecipheringFunctionName(const std::string &ytPlayerSourceCode) {
    auto match = Regexes::Decipherer_findFunctionName.match(ytPlayerSourceCode);
    auto functionName = match.captured("functionName");

    if (functionName.isEmpty()) {
        throw std::runtime_error("[Decipherer] No function name found !");
    }

    return functionName;
}

std::vector<std::string> AudioTube::SignatureDecipherer::_findJSDecipheringOperations(const std::string &ytPlayerSourceCode, const YTClientMethod &obfuscatedDecipheringFunctionName) {
    // get the body of the function
    auto regex = Regexes::Decipherer_findJSDecipheringOperations(obfuscatedDecipheringFunctionName);
    auto match = regex.match(ytPlayerSourceCode);

    auto functionBody = match.captured("functionBody");
    if (functionBody.isEmpty()) {
        throw std::runtime_error("[Decipherer] No function body found !");
    }

    // calls
    auto javascriptFunctionCalls = AudioTube::splitString(functionBody, ';');

    return std::move(javascriptFunctionCalls);
}

std::unordered_map<AudioTube::CipherOperation, AudioTube::SignatureDecipherer::YTClientMethod> AudioTube::SignatureDecipherer::
    _findObfuscatedDecipheringOperationsFunctionName(const std::string &ytPlayerSourceCode, const std::vector<std::string> &javascriptDecipheringOperations) {
    // define out
    std::unordered_map<CipherOperation, YTClientMethod> functionNamesByOperation;

    // find subjacent functions names used by decipherer
    std::set<std::string> uniqueOperations;
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
            auto co = i->first;
            if (functionNamesByOperation.contains(co)) continue;

            // check with regex
            auto regex = i->second;
            if (regex.match(ytPlayerSourceCode).hasMatch()) {
                functionNamesByOperation.emplace(co, calledFunctionName);
                break;
            }
        }
    }

    if (!functionNamesByOperation.size()) {
        throw std::runtime_error("[Decipherer] Missing function names by operations !");
    }

    return functionNamesByOperation;
}

AudioTube::SignatureDecipherer::YTDecipheringOperations AudioTube::SignatureDecipherer::_buildOperations(
        const std::unordered_map<CipherOperation, YTClientMethod> &functionNamesByOperation,
        const std::vector<std::string> &javascriptOperations
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
                operations.push({operationType, -1});  // Argument is meaningless
            }
            break;

            case CipherOperation::Slice:
            case CipherOperation::Swap: {
                operations.push({operationType, arg});
            }
            break;

            default:
                break;
        }
    }

    if (!operations.size()) {
        throw std::runtime_error("[Decipherer] No operations found !");
    }

    return operations;
}

AudioTube::SignatureDecipherer::SignatureDecipherer(const std::string &ytPlayerSourceCode) {
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
