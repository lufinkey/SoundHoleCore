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
		};
		
		static $<Artist> new$(MediaProvider* provider, Data data);
		Artist(MediaProvider* provider, Data data);
		Artist(Json json, FromJsonOptions options);
		
		const Optional<String>& description() const;
		
		virtual bool needsData() const override;
		virtual Promise<void> fetchMissingData() override;
		
		Data toData() const;
		
		static $<Artist> fromJson(Json json, FromJsonOptions options);
		virtual Json toJson() const override;
		
	protected:
		Optional<String> _description;
	};
}
