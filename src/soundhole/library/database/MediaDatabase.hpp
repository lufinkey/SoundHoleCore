//
//  MediaDatabase.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <string>
#include <map>
#include <soundhole/common.hpp>
#include <soundhole/media/MediaProviderStash.hpp>

struct sqlite3;

namespace sh {
	class MediaDatabase {
	public:
		struct Options {
			String name;
			String location;
			MediaProviderStash* stash;
		};
		MediaDatabase(Options);
		~MediaDatabase();
		
		void open();
		bool isOpen() const;
		void close();
		
		Promise<void> initialize();
		Promise<void> reset();
		
		struct CacheOptions {
			std::map<std::string,std::string> dbState;
		};
		
	private:
		Options options;
		sqlite3* db;
		DispatchQueue* queue;
	};
}
