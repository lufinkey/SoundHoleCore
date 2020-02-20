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
	_duration(data.duration), _audioSources(data.audioSources), _playable(data.playable) {
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

	Optional<Track::AudioSource> Track::findAudioSource(FindAudioSourceOptions options) const {
		if(!_audioSources) {
			return std::nullopt;
		}
		Optional<Track::AudioSource> bestAudioSource;
		for(auto& audioSource : _audioSources.value()) {
			if(!bestAudioSource) {
				bestAudioSource = audioSource;
				continue;
			}
			if(audioSource.bitrate >= options.bitrate) {
				bool needVideo = (options.video.has_value() && options.video.value());
				if(needVideo) {
					if(audioSource.videoBitrate.has_value() && !bestAudioSource->videoBitrate.has_value()) {
						bestAudioSource = audioSource;
						continue;
					}
				} else {
					if(!audioSource.videoBitrate.has_value() && bestAudioSource->videoBitrate.has_value()) {
						bestAudioSource = audioSource;
						continue;
					}
				}
				if(audioSource.bitrate < bestAudioSource->bitrate) {
					bestAudioSource = audioSource;
					continue;
				}
				if(options.videoBitrate.has_value() && audioSource.videoBitrate.has_value() && bestAudioSource->videoBitrate.has_value()) {
					if(audioSource.videoBitrate.value() >= options.videoBitrate.value()
					   && (bestAudioSource->videoBitrate.value() < options.videoBitrate.value()
						   || audioSource.videoBitrate.value() < bestAudioSource->videoBitrate.value())) {
						bestAudioSource = audioSource;
						continue;
					}
				}
			} else if(audioSource.bitrate >= bestAudioSource->bitrate) {
				bestAudioSource = audioSource;
				continue;
			}
		}
		if(!options.allowFallback && bestAudioSource) {
			if(bestAudioSource->bitrate < options.bitrate
			   || (options.video.has_value() && options.video.value() != bestAudioSource->videoBitrate.has_value())
			   || (options.videoBitrate.has_value() && bestAudioSource->videoBitrate.has_value() && bestAudioSource->videoBitrate.value() < options.videoBitrate.value())) {
				return std::nullopt;
			}
		}
		return bestAudioSource;
	}

	bool Track::isPlayable() const {
		return _playable;
	}



	bool Track::needsData() const {
		return (!_duration.has_value() || (provider->player()->usesPublicAudioStreams() && !_audioSources.has_value()));
	}

	Promise<void> Track::fetchMissingData() {
		return provider->getTrackData(_uri).then([=](Data data) {
			_albumName = data.albumName;
			_albumURI = data.albumURI;
			_tags = data.tags;
			_discNumber = data.discNumber;
			_trackNumber = data.trackNumber;
			_duration = data.duration;
			_audioSources = data.audioSources;
		});
	}



	Track::Data Track::toData() const {
		return Track::Data{
			MediaItem::toData(),
			.albumName=_albumName,
			.albumURI=_albumURI,
			.artists=_artists,
			.tags=_tags,
			.discNumber=_discNumber,
			.trackNumber=_trackNumber,
			.duration=_duration,
			.audioSources=_audioSources,
			.playable=_playable
		};
	}




	Track::AudioSource Track::AudioSource::fromJson(Json json) {
		auto url = json["url"];
		auto encoding = json["encoding"];
		auto bitrate = json["bitrate"];
		auto videoBitrate = json["videoBitrate"];
		if(!url.is_string() || !encoding.is_string() || !bitrate.is_number() || (!videoBitrate.is_null() && !videoBitrate.is_number())) {
			throw std::invalid_argument("invalid json for Track::AudioSource");
		}
		return AudioSource{
			.url=url.string_value(),
			.encoding=encoding.string_value(),
			.bitrate=(double)bitrate.number_value(),
			.videoBitrate=(!videoBitrate.is_null()) ? maybe((double)videoBitrate.number_value()) : std::nullopt
		};
	}

	Json Track::AudioSource::toJson() const {
		return Json::object{
			{"url",(std::string)url},
			{"encoding",(std::string)encoding},
			{"bitrate",bitrate},
			{"videoBitrate",(videoBitrate ? Json(videoBitrate.value()) : Json())}
		};
	}
}
