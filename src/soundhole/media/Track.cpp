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
	$<Track> Track::new$(MediaProvider* provider, const Data& data) {
		return fgl::new$<Track>(provider, data);
	}
	
	Track::Track(MediaProvider* provider, const Data& data)
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
	
	Optional<Track::AudioSource> Track::findAudioSource(const FindAudioSourceOptions& options) const {
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



	Promise<void> Track::fetchData() {
		auto self = std::static_pointer_cast<Track>(shared_from_this());
		return provider->getTrackData(_uri).then([=](Data data) {
			self->applyData(data);
		});
	}

	void Track::applyData(const Data& data) {
		MediaItem::applyData(data);
		_albumName = data.albumName;
		_albumURI = data.albumURI;
		_tags = data.tags;
		if(data.discNumber) {
			_discNumber = data.discNumber;
		}
		if(data.trackNumber) {
			_trackNumber = data.trackNumber;
		}
		if(data.duration) {
			_duration = data.duration;
		}
		if(data.audioSources) {
			_audioSources = data.audioSources;
		}
		_playable = data.playable;
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




	Track::AudioSource Track::AudioSource::fromJson(const Json& json) {
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



	$<Track> Track::fromJson(const Json& json, MediaProviderStash* stash) {
		return fgl::new$<Track>(json, stash);
	}

	Track::Track(const Json& json, MediaProviderStash* stash)
	: MediaItem(json, stash) {
		auto albumName = json["albumName"];
		auto albumURI = json["albumURI"];
		auto artists = json["artists"];
		auto tags = json["tags"];
		auto discNumber = json["discNumber"];
		auto trackNumber = json["trackNumber"];
		auto duration = json["duration"];
		auto audioSources = json["audioSources"];
		auto playable = json["playable"];
		if(!albumName.is_string() || !albumURI.is_string() || !artists.is_array() || (!tags.is_null() && !tags.is_array())
		   || (!discNumber.is_null() && !discNumber.is_number()) || (!trackNumber.is_null() && !trackNumber.is_number())
		   || (!duration.is_null() && !duration.is_number()) || (!audioSources.is_null() && !audioSources.is_array())
		   || (!playable.is_bool() && !playable.is_number())) {
			throw std::invalid_argument("invalid json for Track");
		}
		_albumName = albumName.string_value();
		_albumURI = albumURI.string_value();
		_artists = ArrayList<$<Artist>>();
		_artists.reserve(artists.array_items().size());
		for(auto artist : artists.array_items()) {
			_artists.pushBack(Artist::fromJson(artist, stash));
		}
		if(tags.is_null()) {
			_tags = std::nullopt;
		} else {
			_tags = ArrayList<String>();
			_tags->reserve(tags.array_items().size());
			for(auto tag : tags.array_items()) {
				_tags->pushBack(tag.string_value());
			}
		}
		_discNumber = (!discNumber.is_null()) ? maybe((size_t)discNumber.number_value()) : std::nullopt;
		_trackNumber = (!trackNumber.is_null()) ? maybe((size_t)trackNumber.number_value()) : std::nullopt;
		_duration = (!duration.is_null()) ? maybe(duration.number_value()) : std::nullopt;
		if(audioSources.is_null()) {
			_audioSources = std::nullopt;
		} else {
			_audioSources = ArrayList<AudioSource>();
			_audioSources->reserve(audioSources.array_items().size());
			for(auto audioSource : audioSources.array_items()) {
				_audioSources->pushBack(AudioSource::fromJson(audioSource));
			}
		}
		_playable = playable.bool_value();
	}

	Json Track::toJson() const {
		auto json = MediaItem::toJson().object_items();
		json.merge(Json::object{
			{"albumName",(std::string)_albumName},
			{"albumURI",(std::string)_albumURI},
			{"artists",Json(_artists.map<Json>([&](auto& artist) {
				return artist->toJson();
			}))},
			{"tags",(_tags ? Json(_tags->map<Json>([&](auto& tag) {
				return Json((std::string)tag);
			})) : Json())},
			{"discNumber",(_discNumber ? Json((double)_discNumber.value()) : Json())},
			{"trackNumber",(_trackNumber ? Json((double)_trackNumber.value()) : Json())},
			{"duration",(_duration ? Json(_duration.value()) : Json())},
			{"audioSources",(_audioSources ? Json(_audioSources->map<Json>([&](auto& audioSource) {
				return audioSource.toJson();
			})) : Json())},
			{"playable",_playable}
		});
		return json;
	}
}
