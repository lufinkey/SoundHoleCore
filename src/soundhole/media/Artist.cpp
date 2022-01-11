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

	#pragma mark Artist::Data

	Artist::Data Artist::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto mediaItemData = MediaItem::Data::fromJson(json, stash);
		auto description = json["description"];
		if((!description.is_null() && !description.is_string())) {
			throw std::invalid_argument("invalid json for Artist");
		}
		return Artist::Data{
			mediaItemData,
			.musicBrainzID = json["musicBrainzID"].string_value(),
			.description = (!description.is_null()) ? maybe((String)description.string_value()) : std::nullopt
		};
	}



	#pragma mark Artist

	$<Artist> Artist::new$(MediaProvider* provider, const Data& data) {
		return fgl::new$<Artist>(provider, data);
	}

	Artist::Artist(MediaProvider* provider, const Data& data)
	: MediaItem(provider, data),
	_musicBrainzID(data.musicBrainzID),
	_description(data.description) {
		//
	}

	const String& Artist::musicBrainzID() const {
		return _musicBrainzID;
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
		if(!data.musicBrainzID.empty()) {
			_musicBrainzID = data.musicBrainzID;
		}
		if(data.description) {
			_description = data.description;
		}
	}

	void Artist::applyDataFrom($<const Artist> artist) {
		applyData(artist->toData());
	}

	bool Artist::isSameClassAs($<const Artist> artist) const {
		return true;
	}

	Artist::Data Artist::toData() const {
		return Artist::Data{
			MediaItem::toData(),
			.musicBrainzID = _musicBrainzID,
			.description=_description
		};
	}

	Json Artist::toJson() const {
		auto json = MediaItem::toJson().object_items();
		json.merge(Json::object{
			{ "musicBrainzID", _musicBrainzID.empty() ? Json() : Json((std::string)_musicBrainzID) },
			{ "description", (_description ? (std::string)_description.value() : Json()) }
		});
		return json;
	}
}
