//
//  UserAccount.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/25/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "MediaItem.hpp"

namespace sh {
	class MediaProvider;

	class UserAccount: public MediaItem {
	public:
		struct Data: public MediaItem::Data {
			String id;
			Optional<String> displayName;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<UserAccount> new$(MediaProvider* provider, const Data& data);
		
		UserAccount(MediaProvider* provider, const Data& data);
		
		const String& id() const;
		const Optional<String>& displayName() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData() const;
		
		virtual Json toJson() const override;
		
	protected:
		String _id;
		Optional<String> _displayName;
	};
}
