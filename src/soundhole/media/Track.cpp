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

	#pragma mark Track::AudioSource

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
			{"url", (std::string)url},
			{"encoding", (std::string)encoding},
			{"bitrate", bitrate},
			{"videoBitrate", (videoBitrate ? videoBitrate.value() : Json())}
		};
	}



	#pragma mark Track::Data

	Track::Data Track::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto mediaItemData = MediaItem::Data::fromJson(json, stash);
		auto albumName = json["albumName"];
		if(!albumName.is_string()) {
			throw std::invalid_argument("invalid json for Track: 'albumName' is required");
		}
		auto albumURI = json["albumURI"];
		if(!albumURI.is_string()) {
			throw std::invalid_argument("invalid json for Track: 'albumURI' is required");
		}
		auto artists = json["artists"];
		if(!artists.is_array()) {
			throw std::invalid_argument("invalid json for Track: 'artists' is required");
		}
		auto tags = json["tags"];
		if(!tags.is_null() && !tags.is_array()) {
			throw std::invalid_argument("invalid json for Track: 'tags' must be an array or null");
		}
		auto discNumber = json["discNumber"];
		if(!discNumber.is_null() && !discNumber.is_number()) {
			throw std::invalid_argument("invalid json for Track: 'discNumber' must be a numnber or null");
		}
		auto trackNumber = json["trackNumber"];
		if(!trackNumber.is_null() && !trackNumber.is_number()) {
			throw std::invalid_argument("invalid json for Track: 'trackNumber' must be a number or null");
		}
		auto duration = json["duration"];
		if(!duration.is_null() && !duration.is_number()) {
			throw std::invalid_argument("invalid json for Track: 'duration' must be a number or null");
		}
		auto audioSources = json["audioSources"];
		if(!audioSources.is_null() && !audioSources.is_array()) {
			throw std::invalid_argument("invalid json for Track: 'audioSources' must be an array or null");
		}
		auto playable = json["playable"];
		if(!playable.is_null() && !playable.is_bool() && !playable.is_number()) {
			throw std::invalid_argument("invalid json for Track: 'playable' must be a bool, a number, or null");
		}
		return Track::Data{
			mediaItemData,
			.albumName = albumName.string_value(),
			.albumURI = albumURI.string_value(),
			.artists = ArrayList<Json>(artists.array_items()).map([=](auto& artistJson) -> $<Artist> {
				auto providerName = artistJson["provider"];
				if(!providerName.is_string()) {
					throw std::invalid_argument("artist provider must be a string");
				}
				auto provider = stash->getMediaProvider(providerName.string_value());
				if(provider == nullptr) {
					throw std::invalid_argument("invalid provider name for artist: "+providerName.string_value());
				}
				return provider->artist(Artist::Data::fromJson(artistJson, stash));
			}),
			.tags = ArrayList<Json>(tags.array_items()).map([](auto& tagJson) -> String {
				return tagJson.string_value();
			}),
			.discNumber = (!discNumber.is_null()) ? maybe((size_t)discNumber.number_value()) : std::nullopt,
			.trackNumber = (!trackNumber.is_null()) ? maybe((size_t)trackNumber.number_value()) : std::nullopt,
			.duration = (!duration.is_null()) ? maybe(duration.number_value()) : std::nullopt,
			.audioSources = ArrayList<Json>(audioSources.array_items()).map([](auto& audioSourceJson) -> AudioSource {
				return AudioSource::fromJson(audioSourceJson);
			}),
			.playable = playable.is_null() ? Optional<bool>() : playable.is_number() ? (playable.number_value() != 0) : playable.bool_value()
		};
	}



	#pragma mark Track

	$<Track> Track::new$(MediaProvider* provider, const Data& data) {
		return fgl::new$<Track>(provider, data);
	}
	
	Track::Track(MediaProvider* provider, const Data& data)
	: MediaItem(provider, data),
	_albumName(data.albumName),
	_albumURI(data.albumURI),
	_artists(data.artists),
	_tags(data.tags),
	_discNumber(data.discNumber),
	_trackNumber(data.trackNumber),
	_duration(data.duration),
	_audioSources(data.audioSources),
	_playable(data.playable) {
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
		return _playable.valueOr(true);
	}

	Optional<bool> Track::playable() const {
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
		if(data.artists.size() > 0) {
			auto newArtists = data.artists;
			for(auto& newArtist : newArtists) {
				if(newArtist->uri().empty()) {
					auto existingArtist = _artists.firstWhere([&](auto& item){
						return !item->name().empty() && item->name() == newArtist->name();
					}, nullptr);
					if(existingArtist != nullptr && !existingArtist->needsData()) {
						newArtist = existingArtist;
					}
				} else {
					auto existingArtist = _artists.firstWhere([&](auto& item){
						return !item->uri().empty() && item->uri() == newArtist->uri();
					}, nullptr);
					if(existingArtist != nullptr && !existingArtist->needsData()) {
						newArtist = existingArtist;
					}
				}
			}
			_artists = newArtists;
		}
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
		if(data.playable.hasValue()) {
			_playable = data.playable;
		}
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

	Json Track::toJson() const {
		auto json = MediaItem::toJson().object_items();
		json.merge(Json::object{
			{"albumName", (std::string)_albumName},
			{"albumURI", (std::string)_albumURI},
			{"artists", Json(_artists.map([&](auto& artist) -> Json {
				return artist->toJson();
			}))},
			{"tags", (_tags ? Json(_tags->map([&](auto& tag) -> Json {
				return Json((std::string)tag);
			})) : Json())},
			{"discNumber", (_discNumber ? Json((double)_discNumber.value()) : Json())},
			{"trackNumber", (_trackNumber ? Json((double)_trackNumber.value()) : Json())},
			{"duration", (_duration ? Json(_duration.value()) : Json())},
			{"audioSources" ,(_audioSources ? Json(_audioSources->map([&](auto& audioSource) -> Json {
				return audioSource.toJson();
			})) : Json())},
			{"playable", _playable.hasValue() ? Json(_playable.value()) : Json()}
		});
		return json;
	}
}
