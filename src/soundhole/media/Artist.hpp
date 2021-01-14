//
//  Artist.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "MediaItem.hpp"

namespace sh {
	class Artist: public MediaItem {
	public:
		struct Data: public MediaItem::Data {
			Optional<String> description;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<Artist> new$(MediaProvider* provider, const Data& data);
		
		Artist(MediaProvider* provider, const Data& data);
		
		const Optional<String>& description() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData() const;
		virtual Json toJson() const override;
		
	protected:
		Optional<String> _description;
	};
}
