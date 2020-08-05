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

#pragma once

#include <string>
#include <utility>
#include <map>
#include <vector>

namespace AudioTube {

class UrlParser {
 public:
    explicit UrlParser(std::string_view rawUrlView);
    std::string host() const;
    std::string scheme() const;
    std::string pathAndQuery() const;
    bool isValid() const;

 private:
    std::string_view _host;
    std::string_view _scheme;
    std::string_view _pathAndQuery;
    bool _isValid = false;
};

class UrlQuery {
 public:
    using Key = std::string;
    using SubQuery = std::string_view;

    explicit UrlQuery(const std::string_view &query);
    UrlQuery(const UrlQuery::Key &key, const UrlQuery::SubQuery &subQuery);
    UrlQuery();

    bool hasSubqueries() const;
    std::string key() const;
    UrlQuery operator[](const UrlQuery::Key &key) const;

    std::vector<UrlQuery> subqueries() const;
    std::string percentDecoded() const;

 private:
    std::map<UrlQuery::Key, UrlQuery::SubQuery> _subqueries;
    std::string _selfKey;
    std::string_view _wholeQuery;
};

// https://www.codeguru.com/cpp/cpp/algorithms/strings/article.php/c12759/URI-Encoding-and-Decoding.htm
class Url {
 public:
    static std::string decode(const std::string & sSrc);
    static std::string encode(const std::string & sSrc);

 private:
    static const char _HEX2DEC[256];
    static const char _SAFE[256];
    static const char _DEC2HEX[16 + 1];
};

}  // namespace AudioTube
