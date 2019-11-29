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
		struct Data: public MediaItem::Data {
			String albumName;
			String albumURI;
			ArrayList<$<Artist>> artists;
			Optional<ArrayList<String>> tags;
			Optional<size_t> discNumber;
			Optional<size_t> trackNumber;
			Optional<double> duration;
		};
		
		static $<Track> new$(MediaProvider* provider, Data data);
		
		Track(MediaProvider* provider, Data data);
		
		const String& albumName() const;
		const String& albumURI() const;
		
		const ArrayList<$<Artist>>& artists();
		const ArrayList<$<const Artist>>& artists() const;
		
		const Optional<ArrayList<String>>& tags() const;
		
		const Optional<size_t>& discNumber() const;
		const Optional<size_t>& trackNumber() const;
		
		const Optional<double>& duration() const;
		
		virtual bool needsData() const override;
		virtual Promise<void> fetchMissingData() override;
		
	protected:
		String _albumName;
		String _albumURI;
		ArrayList<$<Artist>> _artists;
		Optional<ArrayList<String>> _tags;
		Optional<size_t> _discNumber;
		Optional<size_t> _trackNumber;
		Optional<double> _duration;
	};
}
