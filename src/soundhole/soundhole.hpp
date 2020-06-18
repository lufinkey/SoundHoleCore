//
//  soundhole.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/library/MediaLibrary.hpp>
#include <soundhole/providers/bandcamp/BandcampProvider.hpp>
#include <soundhole/providers/spotify/SpotifyProvider.hpp>
#include <soundhole/providers/youtube/YoutubeProvider.hpp>
#include <soundhole/playback/Player.hpp>
#ifdef __OBJC__
#include <soundhole/playback/Player_objc.hpp>
#endif
#include <soundhole/utils/Utils.hpp>
#include <soundhole/utils/SoundHoleError.hpp>
