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

#include <vector>
#include <string>

#include "_NetworkHelper.h"
#include "_DebugHelper.h"

#include "VideoMetadata.h"
#include "VideoInfos.h"

namespace AudioTube {

class NetworkFetcher : public NetworkHelper {
 public:
    static promise::Promise fromPlaylistUrl(const std::string &url);
    static promise::Promise refreshMetadata(VideoMetadata* toRefresh, bool force = false);
    static bool isStreamAvailable(VideoMetadata* toCheck);

 private:
    static promise::Promise _refreshMetadata(VideoMetadata* metadata);

    static std::vector<std::string> _extractVideoIdsFromHTTPRequest(const DownloadedUtf8 &requestData);
    static std::vector<VideoMetadata*> _videoIdsToMetadataList(const std::vector<std::string> &videoIds);
};

}  // namespace AudioTube
