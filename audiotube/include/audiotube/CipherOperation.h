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

namespace AudioTube {

// Careful, order is important !
enum class CipherOperation {
    CO_Unknown,
    Slice,
    Swap,
    Reverse
};

inline unsigned int qHash(const AudioTube::CipherOperation &key, unsigned int seed = 0) {return unsigned int(key) ^ seed;}

}  // namespace AudioTube
