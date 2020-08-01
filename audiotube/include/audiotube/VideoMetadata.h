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

#include "PlayerConfig.h"
#include "StreamsManifest.h"

namespace AudioTube {

class VideoMetadata {
 public:
    enum InstantiationType {
        InstFromId,
        InstFromUrl
    };
    VideoMetadata(const std::string &IdOrUrl, const InstantiationType &type);

    static VideoMetadata* fromVideoUrl(const std::string &url);
    static VideoMetadata* fromVideoId(const std::string &videoId);

    PlayerConfig::VideoId id() const;
    std::string url() const;
    bool hasFailed() const;
    bool ranOnce() const;

    void setFailure(bool failed);
    void setRanOnce();

    void setPlayerConfig(const PlayerConfig &playerConfig);

    StreamsManifest* audioStreams();
    PlayerConfig* playerConfig();

    // callbacks
    void setOnMetadataFetching(const std::function<void()> &cb);
    void setOnMetadataRefreshed(const std::function<void()> &cb);
    void setOnStreamFailed(const std::function<void()> &cb);

    void OnMetadataFetching();
    void OnMetadataRefreshed();
    void OnStreamFailed();

 private:
    static std::string _urlFromVideoId(const std::string &videoId);

    PlayerConfig::VideoId _videoId;
    std::string _url;
    bool _failed = false;
    bool _ranOnce = false;

    PlayerConfig _playerConfig;
    StreamsManifest _audioStreams;

    std::function<void()> _omf_callback;
    std::function<void()> _omr_callback;
    std::function<void()> _osf_callback;
};

}  // namespace AudioTube
