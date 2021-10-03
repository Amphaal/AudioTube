// AudioTube C++
// C++ fork based on https://github.com/Tyrrrz/YoutubeExplode
// Copyright (C) 2019-2021 Guillaume Vara

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "UrlParser.h"

AudioTube::UrlParser::UrlParser(std::string_view rawUrlView) {
    // find scheme separator
    std::string schemeSeparator("://");
    auto findSS = rawUrlView.find(schemeSeparator);
    if (findSS == std::string::npos || findSS == 0) return;
    this->_scheme = rawUrlView.substr(0, findSS);

    this->_isValid = true;

    // find first slash (host)
    auto afterSchemeSeparatorPos = findSS + schemeSeparator.length();
    auto findFirstSlash = rawUrlView.find("/", afterSchemeSeparatorPos);

    if (findFirstSlash == 0) return;
    if (findFirstSlash == std::string::npos) {
        this->_host = rawUrlView.substr(afterSchemeSeparatorPos);
        this->_pathAndQuery = "/";
        return;
    }

    auto pathStartIndex = findFirstSlash - afterSchemeSeparatorPos;
    this->_host = rawUrlView.substr(afterSchemeSeparatorPos, pathStartIndex);

    // else is path + query
    this->_pathAndQuery = rawUrlView.substr(findFirstSlash);
}

bool AudioTube::UrlParser::isValid() const {
    return this->_isValid;
}

std::string AudioTube::UrlParser::host() const {
    return std::string { this->_host };
}

std::string AudioTube::UrlParser::scheme() const {
    return std::string{ this->_scheme };
}

std::string AudioTube::UrlParser::pathAndQuery() const {
    return std::string{ this->_pathAndQuery };
}

AudioTube::UrlQuery::UrlQuery() { }
AudioTube::UrlQuery::UrlQuery(const UrlQuery::Key &key, const UrlQuery::SubQuery &subQuery) : UrlQuery(subQuery) {
    this->_selfKey = key;
}
AudioTube::UrlQuery::UrlQuery(const std::string_view &query) {
    this->_wholeQuery = query;

    // setup for search
    auto keyFindStart = this->_wholeQuery.begin();
    auto valFindStart = keyFindStart;
    UrlQuery::Key currentKey;
    enum {
        FIND_KEY,
        FIND_VALUE
    };
    auto finderFunc = FIND_KEY;

    // search
    for (auto currentChar = keyFindStart; currentChar != this->_wholeQuery.end(); currentChar++) {
        switch (finderFunc) {
            case FIND_KEY: {
                if (*currentChar != *"=") continue;

                currentKey = std::string(keyFindStart, currentChar);

                valFindStart = currentChar + 1;
                finderFunc = FIND_VALUE;
            }
            break;

            case FIND_VALUE: {
                if (*currentChar != *"&") continue;

                UrlQuery::SubQuery subq(&*valFindStart, currentChar - valFindStart);  // &* to allow MSVC (https://github.com/abseil/abseil-cpp/issues/161)
                this->_subqueries.emplace(currentKey, subq);

                keyFindStart = currentChar + 1;
                finderFunc = FIND_KEY;
            }
            break;
        }
    }

    // end FIND_VALUE
    if (finderFunc == FIND_VALUE) {
        UrlQuery::SubQuery subq(&*valFindStart, this->_wholeQuery.end() - valFindStart);   // &* to allow MSVC (https://github.com/abseil/abseil-cpp/issues/161)
        this->_subqueries.emplace(currentKey, subq);
    }
}

std::string AudioTube::UrlQuery::key() const {
    return this->_selfKey;
}

bool AudioTube::UrlQuery::hasSubqueries() const {
    return this->_subqueries.size();
}

AudioTube::UrlQuery AudioTube::UrlQuery::operator[](const UrlQuery::Key &key) const {
    auto keyFound = this->_subqueries.find(key);
    if (keyFound == this->_subqueries.end()) return UrlQuery();
    return UrlQuery(key, keyFound->second);
}

std::vector<AudioTube::UrlQuery> AudioTube::UrlQuery::subqueries() const {
    std::vector<AudioTube::UrlQuery> out;
    for (auto &[k, v] : this->_subqueries) {
        out.emplace(out.end(), k, v);
    }
    return out;
}

std::string AudioTube::UrlQuery::percentDecoded() const {
    return AudioTube::Url::decode(this->undecoded());
}

std::string AudioTube::UrlQuery::undecoded() const {
    return std::string { this->_wholeQuery };
}

std::string AudioTube::Url::decode(const std::string & sSrc) {
    // Note from RFC1630:  "Sequences which start with a percent sign
    // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
    // for future extension"

    const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
    const int SRC_LEN = sSrc.length();
    const unsigned char * const SRC_END = pSrc + SRC_LEN;
    const unsigned char * const SRC_LAST_DEC = SRC_END - 2;  // last decodable '%'

    char * const pStart = new char[SRC_LEN];
    char * pEnd = pStart;

    while (pSrc < SRC_LAST_DEC) {
        if (*pSrc == '%') {
            char dec1, dec2;
            if (-1 != (dec1 = _HEX2DEC[*(pSrc + 1)]) && -1 != (dec2 = _HEX2DEC[*(pSrc + 2)])) {
                *pEnd++ = (dec1 << 4) + dec2;
                pSrc += 3;
                continue;
            }
        }

        *pEnd++ = *pSrc++;
    }

    // the last 2- chars
    while (pSrc < SRC_END)
        *pEnd++ = *pSrc++;

    std::string sResult(pStart, pEnd);
    delete [] pStart;
    return sResult;
}

std::string AudioTube::Url::encode(const std::string & sSrc) {
    const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
    const int SRC_LEN = sSrc.length();
    unsigned char * const pStart = new unsigned char[SRC_LEN * 3];
    unsigned char * pEnd = pStart;
    const unsigned char * const SRC_END = pSrc + SRC_LEN;

    for (; pSrc < SRC_END; ++pSrc) {
        if (_SAFE[*pSrc]) {
            *pEnd++ = *pSrc;
        } else {
            // escape this char
            *pEnd++ = '%';
            *pEnd++ = _DEC2HEX[*pSrc >> 4];
            *pEnd++ = _DEC2HEX[*pSrc & 0x0F];
        }
    }

    std::string sResult((char *)pStart, (char *)pEnd);
    delete [] pStart;
    return sResult;
}

// Only alphanum is safe.
const char AudioTube::Url::_SAFE[256] = {
    /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
    /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,

    /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
    /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,

    /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

    /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

const char AudioTube::Url::_HEX2DEC[256] = {
    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
    /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,

    /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

const char AudioTube::Url::_DEC2HEX[16 + 1] = "0123456789ABCDEF";
