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
		};
		
		static $<UserAccount> new$(MediaProvider* provider, Data data);
		
		UserAccount(MediaProvider* provider, Data data);
		
		const String& id() const;
		const Optional<String>& displayName() const;
		
		virtual bool needsData() const;
		virtual Promise<void> fetchMissingData();
		
	private:
		String _id;
		Optional<String> _displayName;
	};
}
