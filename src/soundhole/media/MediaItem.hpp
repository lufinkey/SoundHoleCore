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
			struct Dimensions {
				size_t width;
				size_t height;
				Size toSize() const;
			};
			
			String url;
			Size size;
			Optional<Dimensions> dimensions;
		};
		
		struct Data {
			String type;
			String name;
			String uri;
			Optional<ArrayList<Image>> images;
		};
		
		MediaItem(MediaProvider* provider, Data data);
		virtual ~MediaItem();
		
		const String& type() const;
		
		const String& name() const;
		const String& uri() const;
		
		const Optional<ArrayList<Image>>& images() const;
		Optional<Image> image(Image::Size size, bool allowFallback=true) const;
		
		MediaProvider* mediaProvider() const;
		
		virtual bool needsData() const = 0;
		virtual Promise<void> fetchMissingData() = 0;
		Promise<void> fetchMissingDataIfNeeded();
		
	protected:
		MediaProvider* provider;
		String _type;
		String _name;
		String _uri;
		Optional<ArrayList<Image>> _images;
		Optional<Promise<void>> _itemDataPromise;
	};
}
