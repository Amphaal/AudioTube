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

#include <list>
#include <string>

#include "_NetworkHelper.h"
#include "_DebugHelper.h"

#include "VideoMetadata.h"
#include "VideoInfos.h"

namespace AudioTube {

class NetworkFetcher : public NetworkHelper {
 public:
    static promise::Defer fromPlaylistUrl(const std::string &url);
    static promise::Defer refreshMetadata(VideoMetadata* toRefresh, bool force = false);
    static void isStreamAvailable(VideoMetadata* toCheck, bool* checkEnded = nullptr, std::string* urlSuccessfullyRequested = nullptr);

 private:
    static promise::Defer _refreshMetadata(VideoMetadata* metadata);

    static std::list<std::string> _extractVideoIdsFromHTTPRequest(const DownloadedUtf8 &requestData);
    static std::list<VideoMetadata*> _videoIdsToMetadataList(const std::list<std::string> &videoIds);
};

}  // namespace AudioTube
