//
//  Track.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "MediaItem.hpp"

namespace sh {
	class Album;
	class Artist;

	class Track: public MediaItem {
	public:
		using MediaItem::MediaItem;
		
		virtual $<Album> album() = 0;
		virtual $<const Album> album() const = 0;
		
		virtual ArrayList<$<Artist>> artists() = 0;
		virtual ArrayList<$<const Artist>> artists() const = 0;
		
		virtual Optional<ArrayList<String>> tags() const = 0;
		
		virtual Optional<size_t> diskNumber() const = 0;
		virtual Optional<size_t> trackNumber() const = 0;
		
		virtual Optional<double> duration() const = 0;
		
		virtual bool isSingle() const = 0;
	};
}
