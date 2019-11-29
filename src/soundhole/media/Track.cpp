//
//  Track.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Track.hpp"
#include "MediaProvider.hpp"

namespace sh {
	$<Track> Track::new$(MediaProvider* provider, Data data) {
		return fgl::new$<Track>(provider, data);
	}

	Track::Track(MediaProvider* provider, Data data)
	: MediaItem(provider, data),
	_albumName(data.albumName), _albumURI(data.albumURI), _artists(data.artists),
	_tags(data.tags), _discNumber(data.discNumber), _trackNumber(data.trackNumber),
	_duration(data.duration), _audioSources(data.audioSources) {
		//
	}
	
	const String& Track::albumName() const {
		return _albumName;
	}

	const String& Track::albumURI() const {
		return _albumURI;
	}
	
	const ArrayList<$<Artist>>& Track::artists() {
		return _artists;
	}

	const ArrayList<$<const Artist>>& Track::artists() const {
		return *((const ArrayList<$<const Artist>>*)(&_artists));
	}
	
	const Optional<ArrayList<String>>& Track::tags() const {
		return _tags;
	}
	
	const Optional<size_t>& Track::discNumber() const {
		return _discNumber;
	}

	const Optional<size_t>& Track::trackNumber() const {
		return _trackNumber;
	}
	
	const Optional<double>& Track::duration() const {
		return _duration;
	}

	const Optional<ArrayList<Track::AudioSource>>& Track::audioSources() const {
		return _audioSources;
	}



	bool Track::needsData() const {
		return provider->doesTrackNeedData(this);
	}

	Promise<void> Track::fetchMissingData() {
		// TODO implement fetchMissingData
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}
}
