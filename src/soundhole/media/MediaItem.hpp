//
//  MediaItem.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "MediaProviderStash.hpp"

namespace sh {
	class MediaProvider;

	CREATE_HAS_STATIC_FUNC(fromJson)
	
	class MediaItem: public std::enable_shared_from_this<MediaItem> {
	public:
		struct Image {
			enum class Size {
				TINY=0,
				SMALL=1,
				MEDIUM=2,
				LARGE=3
			};
			static Size Size_fromJson(const Json& json);
			static Json Size_toJson(const Size& size);
			
			struct Dimensions {
				size_t width;
				size_t height;
				Size toSize() const;
				
				static Dimensions fromJson(const Json& json);
				Json toJson() const;
			};
			
			String url;
			Size size;
			Optional<Dimensions> dimensions;
			
			static Image fromJson(const Json& json);
			Json toJson() const;
		};
		
		struct Data {
			bool partial;
			String type;
			String name;
			String uri;
			Optional<ArrayList<Image>> images;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		MediaItem(MediaProvider* provider, const Data& data);
		virtual ~MediaItem();
		
		const String& type() const;
		
		const String& name() const;
		const String& uri() const;
		
		virtual const Optional<ArrayList<Image>>& images() const;
		Optional<Image> image(Image::Size size, bool allowFallback=true) const;
		
		MediaProvider* mediaProvider() const;
		
		bool needsData() const;
		virtual Promise<void> fetchData() = 0;
		Promise<void> fetchDataIfNeeded();
		void applyData(const Data& data);
		
		Data toData() const;
		virtual Json toJson() const;
		
	protected:
		MediaProvider* provider;
		bool _partial;
		String _type;
		String _name;
		String _uri;
		Optional<ArrayList<Image>> _images;
		Optional<Promise<void>> _itemDataPromise;
	};
}
