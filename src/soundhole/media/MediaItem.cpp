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

	Optional<MediaItem::Image> MediaItem::image(Image::Size size, bool allowFallback) const {
		auto images = this->images();
		if(!images) {
			return std::nullopt;
		}
		Optional<MediaItem::Image> img;
		for(auto& cmpImg : images.value()) {
			if(!img) {
				img = cmpImg;
				continue;
			}
			if(img->size < size) {
				if(cmpImg.size > img->size) {
					img = cmpImg;
				} else if(cmpImg.size == img->size) {
					if(cmpImg.dimensions && img->dimensions && cmpImg.dimensions->height > img->dimensions->height) {
						img = cmpImg;
					}
				}
			} else if(img->size > size) {
				if(cmpImg.size < img->size) {
					if(cmpImg.size >= size) {
						img = cmpImg;
					}
				} else if(cmpImg.size == img->size && cmpImg.dimensions && img->dimensions && cmpImg.dimensions->height < img->dimensions->height) {
					img = cmpImg;
				}
			} else if(cmpImg.size == img->size && cmpImg.dimensions && img->dimensions && cmpImg.dimensions->height > img->dimensions->height) {
				img = cmpImg;
			}
		}
		return img;
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
		if(DispatchQueue::local() != getDefaultPromiseQueue()) {
			return Promise<void>::resolve().then([=]() -> Promise<void> {
				return fetchMissingDataIfNeeded();
			});
		}
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

	MediaItem::Data MediaItem::toData() const {
		return {
			.type=_type,
			.name=_name,
			.uri=_uri,
			.images=_images
		};
	}




	MediaItem::Image::Size MediaItem::Image::Size_fromJson(Json json) {
		if(!json.is_string()) {
			throw std::invalid_argument("invalid json for MediaItem::Image::Size: "+json.string_value());
		}
		auto& str = json.string_value();
		if(str == "TINY") {
			return Size::TINY;
		} else if(str == "SMALL") {
			return Size::SMALL;
		} else if(str == "MEDIUM") {
			return Size::MEDIUM;
		} else if(str == "LARGE") {
			return Size::LARGE;
		} else {
			throw std::invalid_argument("invalid MediaItem::Image::Size " + str);
		}
	}

	Json MediaItem::Image::Size_toJson(Size size) {
		switch(size) {
			case Size::TINY:
				return Json("TINY");
			case Size::SMALL:
				return Json("SMALL");
			case Size::MEDIUM:
				return Json("MEDIUM");
			case Size::LARGE:
				return Json("LARGE");
		}
	}

	MediaItem::Image::Dimensions MediaItem::Image::Dimensions::fromJson(Json json) {
		auto width = json["width"];
		auto height = json["height"];
		if(!width.is_number() || !height.is_number()) {
			throw std::invalid_argument("invalid json for MediaItem::Image::Dimensions");
		}
		return Dimensions{
			.width=(size_t)width.number_value(),
			.height=(size_t)height.number_value()
		};
	}

	Json MediaItem::Image::Dimensions::toJson() const {
		return Json::object{
			{"width", (double)width},
			{"height", (double)height}
		};
	}

	MediaItem::Image MediaItem::Image::fromJson(Json json) {
		auto url = json["url"];
		auto dimensions = json["dimensions"];
		if(!url.is_string() || (!dimensions.is_null() && !dimensions.is_object())) {
			throw std::invalid_argument("invalid json for MediaItem::Image");
		}
		return Image{
			.url=url.string_value(),
			.size=Size_fromJson(json["size"]),
			.dimensions=(!dimensions.is_null()) ? maybe(Dimensions::fromJson(dimensions)) : std::nullopt
		};
	}

	Json MediaItem::Image::toJson() const {
		return Json::object{
			{"url", (std::string)url},
			{"size",Size_toJson(size)},
			{"dimensions",(dimensions ? dimensions->toJson() : Json())}
		};
	}
}
