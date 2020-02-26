//
//  MediaItem.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class MediaProvider;
	
	class MediaItem {
	public:
		struct Image {
			enum class Size {
				TINY=0,
				SMALL=1,
				MEDIUM=2,
				LARGE=3
			};
			static Size Size_fromJson(Json json);
			static Json Size_toJson(Size size);
			
			struct Dimensions {
				size_t width;
				size_t height;
				Size toSize() const;
				
				static Dimensions fromJson(Json json);
				Json toJson() const;
			};
			
			String url;
			Size size;
			Optional<Dimensions> dimensions;
			
			static Image fromJson(Json json);
			Json toJson() const;
		};
		
		struct Data {
			String type;
			String name;
			String uri;
			Optional<ArrayList<Image>> images;
		};
		
		struct FromJsonOptions {
			Function<MediaProvider*(const String&)> providerGetter;
		};
		
		virtual ~MediaItem();
		
		const String& type() const;
		
		const String& name() const;
		const String& uri() const;
		
		virtual const Optional<ArrayList<Image>>& images() const;
		Optional<Image> image(Image::Size size, bool allowFallback=true) const;
		
		MediaProvider* mediaProvider() const;
		
		virtual bool needsData() const = 0;
		virtual Promise<void> fetchMissingData() = 0;
		Promise<void> fetchMissingDataIfNeeded();
		
		Data toData() const;
		virtual Json toJson() const;
		
	protected:
		MediaItem($<MediaItem>& ptr, MediaProvider* provider, Data data);
		MediaItem($<MediaItem>& ptr, Json json, const FromJsonOptions& options);
		
		$<MediaItem> self();
		$<const MediaItem> self() const;
		template<typename T>
		$<T> selfAs();
		template<typename T>
		$<const T> selfAs() const;
		
		w$<MediaItem> weakSelf;
		MediaProvider* provider;
		String _type;
		String _name;
		String _uri;
		Optional<ArrayList<Image>> _images;
		Optional<Promise<void>> _itemDataPromise;
	};




#pragma mark - MediaItem implementation

	template<typename T>
	$<T> MediaItem::selfAs() {
		return std::static_pointer_cast<T>(weakSelf.lock());
	}

	template<typename T>
	$<const T> MediaItem::selfAs() const {
		return std::static_pointer_cast<const T>(weakSelf.lock());
	}
}
