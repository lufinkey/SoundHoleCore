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
			struct Dimensions {
				size_t width;
				size_t height;
			};
			enum class Size {
				TINY,
				SMALL,
				MEDIUM,
				LARGE
			};
			
			String url;
			Size size;
			Optional<Dimensions> dimensions;
		};
		
		struct Data {
			String type;
			String name;
			String uri;
			ArrayList<Image> images;
		};
		
		MediaItem(MediaProvider* provider, Data data);
		virtual ~MediaItem();
		
		const String& type() const;
		
		const String& name() const;
		const String& uri() const;
		
		const ArrayList<Image>& images() const;
		
		MediaProvider* mediaProvider() const;
		
		virtual bool needsData() const = 0;
		virtual Promise<void> fetchMissingData() = 0;
		
	protected:
		MediaProvider* provider;
		String _type;
		String _name;
		String _uri;
		ArrayList<Image> _images;
	};
}
