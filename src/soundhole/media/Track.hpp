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
		struct AudioSource {
			String url;
			String encoding;
			double bitrate;
			Optional<double> videoBitrate;
			
			static AudioSource fromJson(Json json);
			Json toJson() const;
		};
		
		struct Data: public MediaItem::Data {
			String albumName;
			String albumURI;
			ArrayList<$<Artist>> artists;
			Optional<ArrayList<String>> tags;
			Optional<size_t> discNumber;
			Optional<size_t> trackNumber;
			Optional<double> duration;
			Optional<ArrayList<AudioSource>> audioSources;
			bool playable;
		};
		
		static $<Track> new$(MediaProvider* provider, Data data);
		
		const String& albumName() const;
		const String& albumURI() const;
		
		const ArrayList<$<Artist>>& artists();
		const ArrayList<$<const Artist>>& artists() const;
		
		const Optional<ArrayList<String>>& tags() const;
		
		const Optional<size_t>& discNumber() const;
		const Optional<size_t>& trackNumber() const;
		const Optional<double>& duration() const;
		
		const Optional<ArrayList<AudioSource>>& audioSources() const;
		struct FindAudioSourceOptions {
			double bitrate = 128.0;
			Optional<bool> video;
			Optional<double> videoBitrate;
			bool allowFallback = true;
		};
		Optional<AudioSource> findAudioSource(FindAudioSourceOptions options = {.bitrate=128.0,.allowFallback=true}) const;
		bool isPlayable() const;
		
		virtual bool needsData() const override;
		virtual Promise<void> fetchMissingData() override;
		
		Data toData() const;
		
		static $<Track> fromJson(Json json, MediaProviderStash* stash);
		virtual Json toJson() const override;
		
	protected:
		Track($<MediaItem>& ptr, MediaProvider* provider, Data data);
		Track($<MediaItem>& ptr, Json json, MediaProviderStash* stash);
		
		String _albumName;
		String _albumURI;
		ArrayList<$<Artist>> _artists;
		Optional<ArrayList<String>> _tags;
		Optional<size_t> _discNumber;
		Optional<size_t> _trackNumber;
		Optional<double> _duration;
		Optional<ArrayList<AudioSource>> _audioSources;
		bool _playable;
	};
}
