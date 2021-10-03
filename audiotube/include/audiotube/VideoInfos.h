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

#pragma once

#include <string>
#include <sstream>

#include "ATHelper.h"
#include "_NetworkHelper.h"
#include "VideoMetadata.h"
#include "PlayerConfig.h"
#include "SignatureDecipherer.h"
#include "StreamsManifest.h"

namespace AudioTube {

class VideoInfos : public NetworkHelper {
 public:
    static promise::Promise fillStreamsManifest(const PlayerConfig::VideoId &videoId, PlayerConfig* playerConfig, StreamsManifest* manifest);

 private:
    static promise::Promise _downloadRaw_VideoInfos(const PlayerConfig::VideoId &videoId, const std::string &sts);
    static promise::Promise _fillFrom_VideoInfos(const DownloadedUtf8 &dl, StreamsManifest* manifest, PlayerConfig *playerConfig);

    static std::string _percentEncodeUrl(const std::string &rawUrl);
    static std::string _percentDecodeUrl(const std::string &encodedUrl);
};

}  // namespace AudioTube
