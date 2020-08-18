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
    std::smatch matches;
    std::regex_search(ytPlayerSourceCode, matches, Regexes::Decipherer_findFunctionName);

    if (!matches.size()) throw std::runtime_error("[Decipherer] No function name found !");

    auto functionName = matches.str(0);

    return functionName;
}

std::vector<std::string> AudioTube::SignatureDecipherer::_findJSDecipheringOperations(const std::string &ytPlayerSourceCode, const YTClientMethod &obfuscatedDecipheringFunctionName) {
    // get the body of the function
    std::smatch matches;
    auto regex = Regexes::Decipherer_findJSDecipheringOperations(obfuscatedDecipheringFunctionName);
    std::regex_search(ytPlayerSourceCode, matches, regex);

    if (!matches.size()) throw std::runtime_error("[Decipherer] No function body found !");

    // calls
    auto functionBody = matches.str(0);
    auto javascriptFunctionCalls = AudioTube::splitString(functionBody, ';');

    return javascriptFunctionCalls;
}

std::unordered_map<AudioTube::CipherOperation, AudioTube::SignatureDecipherer::YTClientMethod> AudioTube::SignatureDecipherer::
    _findObfuscatedDecipheringOperationsFunctionName(const std::string &ytPlayerSourceCode, const std::vector<std::string> &javascriptDecipheringOperations) {
    // define out
    std::unordered_map<CipherOperation, YTClientMethod> functionNamesByOperation;

    // find subjacent functions names used by decipherer
    std::set<std::string> uniqueOperations;
    for (const auto &call : javascriptDecipheringOperations) {
        // find function name in method call
        std::smatch matches;
        std::regex_search(call, matches, Regexes::Decipherer_findCalledFunction);

        if (!matches.size()) continue;

        auto calledFunctionName = matches.str(0);

        // add to set
        uniqueOperations.insert(calledFunctionName);
    }

    // iterate through unique operations
    for (const auto &calledFunctionName : uniqueOperations) {
        // custom regexes to determine associated decipherer method
        auto customRegexes = Regexes::Decipherer_DecipheringOps(calledFunctionName);

        // find...
        for (auto [co, regex] : customRegexes) {
            // if already found, skip
            if (functionNamesByOperation.find(co) != functionNamesByOperation.end()) continue;

            // check with regex
            std::smatch matches;
            std::regex_search(ytPlayerSourceCode, matches, regex);
            if (matches.size()) {
                functionNamesByOperation.emplace(co, calledFunctionName);
                break;  // found, break
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
        std::smatch matches;
        std::regex_search(call, matches, Regexes::Decipherer_findFuncAndArgument);
        if (!matches.size()) continue;

        auto calledFunctionName = matches.str(0);
        auto arg = safe_stoi(matches.str(1));

        // find associated operation type
        auto operationTypeFound = std::find_if(
            functionNamesByOperation.begin(),
            functionNamesByOperation.end(),
            [calledFunctionName](const auto& mo) {
                return mo.second == calledFunctionName;
            }
        );

        if(operationTypeFound == functionNamesByOperation.end())
            throw std::logic_error("Cannot find associated operation type from serialized JS operation");

        auto operationType = operationTypeFound->first;

        // by operation type
        switch (operationType) {
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
