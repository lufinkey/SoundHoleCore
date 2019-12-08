//
//  MediaItem.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "MediaItem.hpp"

namespace sh {
	MediaItem::Image::Size MediaItem::Image::Dimensions::toSize() const {
		if(height <= 100 || width <= 100) {
			return Size::TINY;
		} else if(height <= 280 || width <= 280) {
			return Size::SMALL;
		} else if(height <= 800 || width <= 800) {
			return Size::MEDIUM;
		} else {
			return Size::LARGE;
		}
	}

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
	
	const Optional<ArrayList<MediaItem::Image>>& MediaItem::images() const {
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

	Promise<void> MediaItem::fetchMissingDataIfNeeded() {
		if(!needsData()) {
			return Promise<void>::resolve();
		}
		if(_itemDataPromise.has_value()) {
			return _itemDataPromise.value();
		}
		auto promise = fetchMissingData().finally([=]() {
			_itemDataPromise.reset();
		});
		_itemDataPromise = promise;
		return promise;
	}
}
