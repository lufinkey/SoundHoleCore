//
//  OmniArtist.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/7/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "OmniArtist.hpp"

namespace sh {
	OmniArtist::Data OmniArtist::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		return OmniArtist::Data{
			Artist::Data::fromJson(json, stash),
			.musicBrainzID = json["musicBrainzID"].string_value(),
			.linkedArtists = ArrayList<$<Artist>>(json["linkedArtists"].array_items(), [&](auto& json) {
				auto mediaItem = stash->parseMediaItem(json);
				if(!mediaItem) {
					throw std::invalid_argument("Invalid json for OmniArtist: elements of linkedArtists cannot be null");
				}
				auto artist = std::dynamic_pointer_cast<Artist>(mediaItem);
				if(!artist) {
					throw std::invalid_argument("Invalid json for OmniArtist: parsed "+mediaItem->type()+" instead of expected type artist in linkedArtists");
				}
				if(std::dynamic_pointer_cast<OmniArtist>(artist)) {
					throw std::invalid_argument("Invalid json for OmniArtist: linked artist of OmniArtist cannot be an OmniArtist");
				}
				return artist;
			})
		};
	}

	$<OmniArtist> OmniArtist::new$(MediaProvider* provider, const Data& data) {
		return fgl::new$<OmniArtist>(provider, data);
	}

	OmniArtist::OmniArtist(MediaProvider* provider, const Data& data)
	: Artist(provider, data), _musicBrainzID(data.musicBrainzID), _linkedArtists(data.linkedArtists) {
		//
	}

	const String& OmniArtist::musicBrainzID() const {
		return _musicBrainzID;
	}

	const ArrayList<$<Artist>>& OmniArtist::linkedArtists() {
		return _linkedArtists;
	}

	const ArrayList<$<const Artist>>& OmniArtist::linkedArtists() const {
		return *((const ArrayList<$<const Artist>>*)(&_linkedArtists));
	}

	Promise<void> OmniArtist::fetchData() {
		return Promise<void>::all(_linkedArtists.map([](auto& artist) {
			return artist->fetchDataIfNeeded();
		})).then([]() {
			// TODO fetch omni artist data (get music brainz ID, link other artists, etc)
		});
	}

	void OmniArtist::applyData(const Data& data) {
		Artist::applyData(data);
		if(!data.musicBrainzID.empty()) {
			_musicBrainzID = data.musicBrainzID;
		}
		if(!data.linkedArtists.empty()) {
			// combine artist arrays
			auto newArtists = _linkedArtists;
			newArtists.reserve(std::max(_linkedArtists.size(), data.linkedArtists.size()));
			// go through data.linkedTracks and apply data to any existing tracks where needed
			for(auto& artist : data.linkedArtists) {
				auto existingArtist = _linkedArtists.firstWhere([&](auto& cmpArtist) {
					return (artist->mediaProvider() == cmpArtist->mediaProvider() && artist->uri() == cmpArtist->uri()
						&& (!artist->uri().empty() || artist->name() == cmpArtist->name()));
				}, nullptr);
				if(existingArtist) {
					// check if we need data or new track isn't partial
					if(existingArtist->needsData() || !artist->needsData()) {
						existingArtist->applyData(artist->toData());
					}
				} else {
					newArtists.pushBack(artist);
				}
			}
			// apply new artists
			_linkedArtists = newArtists;
		}
	}

	OmniArtist::Data OmniArtist::toData() const {
		return Data{
			Artist::toData(),
			.musicBrainzID = _musicBrainzID,
			.linkedArtists = _linkedArtists
		};
	}

	Json OmniArtist::toJson() const {
		Json::object json = Artist::toJson().object_items();
		json["musicBrainzID"] = _musicBrainzID;
		json["linkedArtists"] = _linkedArtists.map([](auto& artist) {
			return artist->toJson();
		});
		return json;
	}
}
