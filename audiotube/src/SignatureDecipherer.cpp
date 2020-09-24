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

void AudioTube::SignatureDecipherer::printOperations() const {
    auto copyOfOperations = this->_operations;
    std::vector<std::string> listOfOps;

    while(!copyOfOperations.empty()) {
        auto [op, arg] = copyOfOperations.front();

        switch (op) {
            case CipherOperation::Reverse: {
                listOfOps.push_back(CipherOperation_str[op]);
            }
            break;

            case CipherOperation::Swap:
            case CipherOperation::Slice: {
                listOfOps.push_back(CipherOperation_str[op] + "(" + std::to_string(arg) + ")");
            }
            break;

            default: {
                throw std::logic_error("Unhandled operation !");
            }
            break;
        }

        copyOfOperations.pop();
    }

    // create output
    std::string out;
    for (const auto &piece : listOfOps) out += piece + ", ";
    if(listOfOps.size()) out = out.substr(0, out.length() - 2);

    spdlog::debug("Decipherer : Operations >> [{}]", out);
}

// for test purposes only !
AudioTube::SignatureDecipherer::SignatureDecipherer(const YTDecipheringOperations &operations) {
    this->_operations = operations;
}

std::string AudioTube::SignatureDecipherer::decipher(const std::string &signature) const {
    auto modifiedSignature = signature;
    auto copyOfOperations = this->_operations;

    while (!copyOfOperations.empty()) {
        auto operationPair = copyOfOperations.front();

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

                modifiedSignature[firstIndex] = second;
                modifiedSignature[secondIndex] = first;
            }
            break;

            default:
                break;
        }

        // spdlog::debug(modifiedSignature);
        copyOfOperations.pop();
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
    jp::VecNum matches;
    jp::RegexMatch rm;
    rm.setRegexObject(&Regexes::Decipherer_findFunctionName)
        .setSubject(&ytPlayerSourceCode)
        .addModifier("gm")
        .setNumberedSubstringVector(&matches)
        .match();

    if (matches.size() != 1) throw std::runtime_error("[Decipherer] No function name found !");

    auto functionName = matches[0][1];

    return functionName;
}

std::vector<std::string> AudioTube::SignatureDecipherer::_findJSDecipheringOperations(const std::string &ytPlayerSourceCode, const YTClientMethod &obfuscatedDecipheringFunctionName) {
    // get the body of the function
    jp::VecNum matches;
    jp::RegexMatch rm;

    auto regex = Regexes::Decipherer_findJSDecipheringOperations(obfuscatedDecipheringFunctionName);
    rm.setRegexObject(&regex)
        .setSubject(&ytPlayerSourceCode)
        .addModifier("gm")
        .setNumberedSubstringVector(&matches)
        .match();

    if (matches.size() == 0) throw std::runtime_error("[Decipherer] No function body found !");

    // calls
    auto functionBody = matches[0][1];  // take first
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
        jp::VecNum matches;
        jp::RegexMatch rm;
        rm.setRegexObject(&Regexes::Decipherer_findCalledFunction)
            .setSubject(&call)
            .addModifier("gm")
            .setNumberedSubstringVector(&matches)
            .match();

        if (matches.size() != 1) continue;

        auto calledFunctionName = matches[0][1];

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
            jp::VecNum matches;
            jp::RegexMatch rm;
            rm.setRegexObject(&regex)
                .setSubject(&ytPlayerSourceCode)
                .addModifier("gm")
                .setNumberedSubstringVector(&matches)
                .match();

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
        const std::vector<std::string> &javascriptOperations) {
    // determine order and execution of subjacent methods
    YTDecipheringOperations operations;

    // iterate
    for (const auto &call : javascriptOperations) {
        // find which function is called
        jp::VecNum matches;
        jp::RegexMatch rm;
        rm.setRegexObject(&Regexes::Decipherer_findFuncAndArgument)
                .setSubject(&call)
                .addModifier("gm")
                .setNumberedSubstringVector(&matches)
                .match();

        if (matches.size() != 1) continue;

        auto calledFunctionName = matches[0][1];
        auto arg = safe_stoi(matches[0][2]);

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
