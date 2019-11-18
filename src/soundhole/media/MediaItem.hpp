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
		
		MediaItem(MediaProvider* provider);
		virtual ~MediaItem();
		
		virtual String type() const = 0;
		
		virtual String name() const = 0;
		virtual String uri() const = 0;
		
		virtual ArrayList<Image> images() const = 0;
		
		virtual MediaProvider* provider() const;
		
		virtual bool needsData() const;
		virtual Promise<void> fetchMissingData();
		
	private:
		MediaProvider* provider;
	};
}
