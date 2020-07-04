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
	$<Artist> Artist::new$(MediaProvider* provider, const Data& data) {
		return fgl::new$<Artist>(provider, data);
	}

	Artist::Artist(MediaProvider* provider, const Data& data)
	: MediaItem(provider, data),
	_description(data.description) {
		//
	}
	
	const Optional<String>& Artist::description() const {
		return _description;
	}



	Promise<void> Artist::fetchData() {
		auto self = std::static_pointer_cast<Artist>(shared_from_this());
		return provider->getArtistData(_uri).then([=](Data data) {
			self->applyData(data);
		});
	}

	void Artist::applyData(const Data& data) {
		MediaItem::applyData(data);
		if(data.description) {
			_description = data.description;
		}
	}



	Artist::Data Artist::toData() const {
		return Artist::Data{
			MediaItem::toData(),
			.description=_description
		};
	}



	$<Artist> Artist::fromJson(const Json& json, MediaProviderStash* stash) {
		return fgl::new$<Artist>(json, stash);
	}

	Artist::Artist(const Json& json, MediaProviderStash* stash)
	: MediaItem(json, stash) {
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
