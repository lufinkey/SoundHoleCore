//
//  MediaProviderStash.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/12/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "MediaProviderStash.hpp"
#include "MediaProvider.hpp"

namespace sh {
	$<MediaItem> MediaProviderStash::parseMediaItem(const Json& json) {
		if(json.is_null()) {
			return nullptr;
		}
		auto providerName = json["provider"];
		if(!providerName.is_string()) {
			throw std::invalid_argument("provider must be a string");
		}
		auto provider = getMediaProvider(providerName.string_value());
		if(provider == nullptr) {
			throw std::invalid_argument("invalid provider name: "+providerName.string_value());
		}
		auto type = json["type"].string_value();
		if(type.empty()) {
			throw std::invalid_argument("No type stored for media item json");
		}
		if(type == "track") {
			return provider->track(Track::Data::fromJson(json,this));
		} else if(type == "artist" || type == "label") {
			return provider->artist(Artist::Data::fromJson(json,this));
		} else if(type == "album") {
			return provider->album(Album::Data::fromJson(json,this));
		} else if(type == "playlist") {
			return provider->playlist(Playlist::Data::fromJson(json,this));
		} else if(type == "user") {
			return provider->userAccount(UserAccount::Data::fromJson(json,this));
		} else if(type == "omni-track") {
			return provider->omniTrack(OmniTrack::Data::fromJson(json,this));
		} else if(type == "omni-artist") {
			return provider->omniArtist(OmniArtist::Data::fromJson(json,this));
		}
		throw std::invalid_argument("invalid media item type "+type);
	}
}
