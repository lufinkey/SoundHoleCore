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
		};
		
		static $<UserAccount> new$(MediaProvider* provider, Data data);
		
		UserAccount(MediaProvider* provider, Data data);
		
		const String& id() const;
		
		virtual bool needsData() const;
		virtual Promise<void> fetchMissingData();
		
	private:
		String _id;
	};
}
