//
//  OmniArtist.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/7/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Artist.hpp"

namespace sh {
	class OmniArtist: public Artist {
	public:
		struct Data: public Artist::Data {
			String musicBrainzID;
			ArrayList<$<Artist>> linkedArtists;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<OmniArtist> new$(MediaProvider* provider, const Data& data);
		OmniArtist(MediaProvider* provider, const Data& data);
		
		const String& musicBrainzID() const;
		
		const ArrayList<$<Artist>>& linkedArtists();
		const ArrayList<$<const Artist>>& linkedArtists() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData() const;
		virtual Json toJson() const override;
		
	private:
		String _musicBrainzID;
		ArrayList<$<Artist>> _linkedArtists;
	};
}
