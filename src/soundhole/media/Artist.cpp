//
//  Artist.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Artist.hpp"
#include "MediaProvider.hpp"

namespace sh {
	$<Artist> Artist::new$(MediaProvider* provider, Data data) {
		$<MediaItem> ptr;
		new Artist(ptr, provider, data);
		return std::static_pointer_cast<Artist>(ptr);
	}

	Artist::Artist(std::shared_ptr<MediaItem>& ptr, MediaProvider* provider, Data data)
	: MediaItem(ptr, provider, data),
	_description(data.description) {
		//
	}
	
	const Optional<String>& Artist::description() const {
		return _description;
	}



	bool Artist::needsData() const {
		// TODO implement Artist::needsData
		return false;
	}

	Promise<void> Artist::fetchMissingData() {
		return provider->getArtistData(_uri).then([=](Data data) {
			_name = data.name;
			_description = data.description;
		});
	}



	Artist::Data Artist::toData() const {
		return Artist::Data{
			MediaItem::toData(),
			.description=_description
		};
	}



	$<Artist> Artist::fromJson(Json json, MediaProviderStash* stash) {
		$<MediaItem> ptr;
		new Artist(ptr, json, stash);
		return std::static_pointer_cast<Artist>(ptr);
	}

	Artist::Artist($<MediaItem>& ptr, Json json, MediaProviderStash* stash)
	: MediaItem(ptr, json, stash) {
		auto description = json["description"];
		if((!description.is_null() && !description.is_string())) {
			throw std::invalid_argument("invalid json for Artist");
		}
		_description = (!description.is_null()) ? maybe((String)description.string_value()) : std::nullopt;
	}

	Json Artist::toJson() const {
		auto json = MediaItem::toJson().object_items();
		json.merge(Json::object{
			{"description",(_description ? Json((std::string)_description.value()) : Json())}
		});
		return json;
	}
}
