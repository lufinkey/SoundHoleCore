//
//  MediaItem.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "MediaItem.hpp"
#include "MediaProvider.hpp"

namespace sh {

	#pragma mark MediaItem::Image

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

	MediaItem::Image::Size MediaItem::Image::Size_fromJson(const Json& json) {
		if(!json.is_string()) {
			throw std::invalid_argument("invalid json for MediaItem::Image::Size: "+json.string_value());
		}
		auto& str = json.string_value();
		if(str == "TINY" || str == "tiny") {
			return Size::TINY;
		} else if(str == "SMALL" || str == "small") {
			return Size::SMALL;
		} else if(str == "MEDIUM" || str == "medium") {
			return Size::MEDIUM;
		} else if(str == "LARGE" || str == "large") {
			return Size::LARGE;
		} else {
			throw std::invalid_argument("invalid MediaItem::Image::Size " + str);
		}
	}

	Json MediaItem::Image::Size_toJson(const Size& size) {
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

	MediaItem::Image::Dimensions MediaItem::Image::Dimensions::fromJson(const Json& json) {
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

	MediaItem::Image MediaItem::Image::fromJson(const Json& json) {
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



	#pragma mark MediaItem::Data

	MediaItem::Data MediaItem::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto partial = json["partial"];
		auto type = json["type"];
		auto name = json["name"];
		auto uri = json["uri"];
		auto images = json["images"];
		if(!type.is_string()) {
			throw std::invalid_argument("invalid json for MediaItem: 'type' is required");
		}
		if(!name.is_string()) {
			throw std::invalid_argument("invalid json for MediaItem: 'name' is required");
		}
		if(!uri.is_string()) {
			throw std::invalid_argument("invalid json for MediaItem: 'uri' is required");
		}
		if((!images.is_null() && !images.is_array())) {
			throw std::invalid_argument("invalid json for MediaItem: 'images' must be an array or null");
		}
		return MediaItem::Data{
			.partial = partial.is_bool() ? partial.bool_value() : true,
			.type = type.string_value(),
			.name = name.string_value(),
			.uri = uri.string_value(),
			.images = (!images.is_null()) ? maybe(ArrayList<Json>(images.array_items()).map([](auto& imgJson) -> Image {
				return Image::fromJson(imgJson);
			})) : std::nullopt
		};
	}



	#pragma mark MediaItem

	MediaItem::MediaItem(MediaProvider* provider, const Data& data)
	: provider(provider),
	_partial(data.partial), _type(data.type), _name(data.name), _uri(data.uri), _images(data.images) {
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
		return _partial;
	}

	Promise<void> MediaItem::fetchDataIfNeeded() {
		w$<MediaItem> weakSelf = shared_from_this();
		if(DispatchQueue::local() != defaultPromiseQueue()) {
			return Promise<void>::resolve().then([=]() -> Promise<void> {
				auto self = weakSelf.lock();
				if(!self) {
					return Promise<void>::resolve();
				}
				return self->fetchDataIfNeeded();
			});
		}
		if(!needsData()) {
			return Promise<void>::resolve();
		}
		if(_itemDataPromise.has_value()) {
			return _itemDataPromise.value();
		}
		auto promise = fetchData().then([=]() {
			auto self = weakSelf.lock();
			if(!self) {
				return;
			}
			self->_partial = false;
		}).finally([=]() {
			auto self = weakSelf.lock();
			if(!self) {
				return;
			}
			self->_itemDataPromise.reset();
		});
		if(!promise.isComplete()) {
			_itemDataPromise = promise;
		}
		return promise;
	}

	void MediaItem::applyData(const Data& data) {
		_partial = data.partial;
		_type = data.type;
		_name = data.name;
		_uri = data.uri;
		if(data.images) {
			_images = data.images;
		}
	}

	MediaItem::Data MediaItem::toData() const {
		return {
			.partial=_partial,
			.type=_type,
			.name=_name,
			.uri=_uri,
			.images=_images
		};
	}

	Json MediaItem::toJson() const {
		return Json::object{
			{"provider",(std::string)provider->name()},
			{"partial",_partial},
			{"type",(std::string)_type},
			{"name",(std::string)_name},
			{"uri",(std::string)_uri},
			{"images",(_images ? Json(_images->map([&](auto& image) -> Json {
				return image.toJson();
			})) : Json())}
		};
	}
}
