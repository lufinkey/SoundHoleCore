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

	MediaItem::MediaItem($<MediaItem>& ptr, MediaProvider* provider, const Data& data)
	: provider(provider),
	_partial(data.partial), _type(data.type), _name(data.name), _uri(data.uri), _images(data.images) {
		ptr = $<MediaItem>(this);
		weakSelf = ptr;
	}
	
	MediaItem::~MediaItem() {
		//
	}



	$<MediaItem> MediaItem::self() {
		return weakSelf.lock();
	}

	$<const MediaItem> MediaItem::self() const {
		return std::static_pointer_cast<const MediaItem>(weakSelf.lock());
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
		auto weakSelf = this->weakSelf;
		if(DispatchQueue::local() != getDefaultPromiseQueue()) {
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



	MediaItem::MediaItem($<MediaItem>& ptr, Json json, MediaProviderStash* stash) {
		ptr = $<MediaItem>(this);
		weakSelf = ptr;
		auto providerName = json["provider"];
		auto partial = json["partial"];
		auto type = json["type"];
		auto name = json["name"];
		auto uri = json["uri"];
		auto images = json["images"];
		if(!providerName.is_string() || !type.is_string() || !name.is_string()
		   || !uri.is_string() || (!images.is_null() && !images.is_array())) {
			throw std::invalid_argument("invalid json for MediaItem");
		}
		provider = stash->getMediaProvider(providerName.string_value());
		if(provider == nullptr) {
			throw std::invalid_argument("invalid provider name: "+providerName.string_value());
		}
		_partial = partial.is_bool() ? partial.bool_value() : true;
		_type = type.string_value();
		_name = name.string_value();
		_uri = uri.string_value();
		if(images.is_null()) {
			_images = std::nullopt;
		} else {
			_images = ArrayList<Image>();
			_images->reserve(images.array_items().size());
			for(auto image : images.array_items()) {
				_images->pushBack(Image::fromJson(image));
			}
		}
	}

	Json MediaItem::toJson() const {
		return Json::object{
			{"provider",(std::string)provider->name()},
			{"partial",_partial},
			{"type",(std::string)_type},
			{"name",(std::string)_name},
			{"uri",(std::string)_uri},
			{"images",(_images ? Json(_images->map<Json>([&](auto& image) {
				return image.toJson();
			})) : Json())}
		};
	}
}
