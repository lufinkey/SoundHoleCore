//
//  MediaItem.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "MediaItem.hpp"

namespace sh {
	MediaItem::MediaItem(MediaProvider* provider, Data data)
	: provider(provider),
	_type(data.type), _name(data.name), _uri(data.uri), _images(data.images) {
		//
	}
	
	MediaItem::~MediaItem() {
		//
	}

	const String& MediaItem::type() const {
		return _type;
	}
	
	const String& MediaItem::name() const {
		return _name;
	}

	const String& MediaItem::uri() const {
		return _uri;
	}
	
	const ArrayList<MediaItem::Image>& MediaItem::images() const {
		return _images;
	}
	
	MediaProvider* MediaItem::mediaProvider() const {
		return provider;
	}

	bool MediaItem::needsData() const {
		return false;
	}

	Promise<void> MediaItem::fetchMissingData() {
		return Promise<void>::resolve();
	}
}
