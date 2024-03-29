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
			String musicBrainzID;
			Optional<String> description;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<Artist> new$(MediaProvider* provider, const Data& data);
		Artist(MediaProvider* provider, const Data& data);
		
		const String& musicBrainzID() const;
		
		const Optional<String>& description() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		virtual void applyDataFrom($<const Artist> artist);
		
		virtual bool isSameClassAs($<const Artist> artist) const;
		
		Data toData() const;
		virtual Json toJson() const override;
		
	protected:
		String _musicBrainzID;
		Optional<String> _description;
	};
}
