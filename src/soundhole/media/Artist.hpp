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
		
		const Optional<String>& description() const;
		
		virtual bool needsData() const override;
		virtual Promise<void> fetchMissingData() override;
		
		Data toData() const;
		
		static $<Artist> fromJson(Json json, const FromJsonOptions& options);
		virtual Json toJson() const override;
		
	protected:
		Artist(std::shared_ptr<MediaItem>& ptr, MediaProvider* provider, Data data);
		Artist(std::shared_ptr<MediaItem>& ptr, Json json, const FromJsonOptions& options);
		
		Optional<String> _description;
	};
}
