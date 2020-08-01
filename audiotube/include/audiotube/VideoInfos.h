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

#include "_NetworkHelper.h"
#include "VideoMetadata.h"
#include "PlayerConfig.h"
#include "SignatureDecipherer.h"
#include "StreamsManifest.h"

namespace AudioTube {

class VideoInfos : public NetworkHelper {
 public:
    static promise::Defer fillStreamsManifest(const PlayerConfig::VideoId &videoId, PlayerConfig* playerConfig, StreamsManifest* manifest);

 private:
    static promise::Defer _downloadRaw_VideoInfos(const PlayerConfig::VideoId &videoId, const std::string &sts);
    static promise::Defer _fillFrom_VideoInfos(const DownloadedUtf8 &dl, StreamsManifest* manifest, PlayerConfig *playerConfig);
};

}  // namespace AudioTube
