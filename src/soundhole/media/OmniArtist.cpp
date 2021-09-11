//
//  OmniArtist.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/7/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "OmniArtist.hpp"
#include "MediaProvider.hpp"

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
		for(auto& artist : data.linkedArtists) {
			if(artist.as<OmniArtist>() != nullptr) {
				throw std::invalid_argument("Linked artist for OmniArtist cannot also be an OmniArtist");
			}
		}
		if(!data.musicBrainzID.empty()) {
			_musicBrainzID = data.musicBrainzID;
		}
		if(!data.linkedArtists.empty()) {
			// combine artist arrays
			auto newArtists = _linkedArtists;
			newArtists.reserve(std::max(_linkedArtists.size(), data.linkedArtists.size()));
			// go through data.linkedArtists and apply data to any existing artists where needed
			for(auto& newArtist : data.linkedArtists) {
				auto existingArtistIt = _linkedArtists.findWhere([&](auto& cmpArtist) {
					return (newArtist->mediaProvider()->name() == cmpArtist->mediaProvider()->name()
						&& ((cmpArtist->uri().empty() && newArtist->name() == cmpArtist->name())
							|| (!cmpArtist->uri().empty() && newArtist->uri() == cmpArtist->uri())));
				});
				if(existingArtistIt != _linkedArtists.end()) {
					auto& existingArtist = *existingArtistIt;
					// if new artist encapsulates the existing artist
					if(newArtist->isSameClassAs(existingArtist)) {
						// apply data only if we need data or new artist isn't partial
						if(existingArtist->needsData() || !newArtist->needsData()) {
							existingArtist->applyDataFrom(newArtist);
						}
					} else {
						// new artist is a different type, so just use new artist
						existingArtist = newArtist;
					}
				} else {
					newArtists.pushBack(newArtist);
				}
			}
			// apply new artists
			_linkedArtists = newArtists;
		}
		// if a linked artist matches this current artist, apply the data there too
		auto matchedArtist = data.uri.empty() ?
			// match artist by name
			_linkedArtists.firstWhere([&](auto& cmpArtist) {
				return (cmpArtist->mediaProvider()->name() == this->mediaProvider()->name() && cmpArtist->uri().empty() && cmpArtist->name() == data.name);
			}, nullptr)
			// match artist by URI (or fallback to name if cmpArtist is missing URI)
			: _linkedArtists.firstWhere([&](auto& cmpArtist) {
				return cmpArtist->mediaProvider()->name() == this->mediaProvider()->name()
				&& (cmpArtist->uri() == data.uri
					|| (cmpArtist->uri().empty() && cmpArtist->name() == data.name));
			}, nullptr);
		if(matchedArtist) {
			matchedArtist->applyDataFrom(_$(shared_from_this()).forceAs<Artist>());
		}
	}

	void OmniArtist::applyDataFrom($<const Artist> artist) {
		if(auto omniArtist = artist.as<const OmniArtist>()) {
			applyData(omniArtist->toData());
		} else {
			Artist::applyDataFrom(artist);
		}
	}

	bool OmniArtist::isSameClassAs($<const Artist> artist) const {
		if(artist.as<const OmniArtist>() == nullptr) {
			return false;
		}
		return true;
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
