//
//  MediaDatabase.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaDatabase.hpp"
#include <sqlite3.h>

namespace sh {
	MediaDatabase::MediaDatabase(Options options)
	: options(options), db(nullptr), queue(nullptr) {
		//
	}

	MediaDatabase::~MediaDatabase() {
		if(isOpen()) {
			close();
		}
	}

	void MediaDatabase::open() {
		if(db != nullptr) {
			return;
		}
		sqlite3* tmpDb;
		int retVal = sqlite3_open(options.location.c_str(), &tmpDb);
		if(retVal != 0) {
			sqlite3_close(tmpDb);
			String errorMsg = sqlite3_errstr(retVal);
			throw std::runtime_error(errorMsg);
		}
		db = tmpDb;
		queue = new DispatchQueue("MediaDatabase");
	}

	bool MediaDatabase::isOpen() const {
		return (db != nullptr);
	}

	void MediaDatabase::close() {
		if(db == nullptr) {
			return;
		}
		delete queue;
		queue = nullptr;
		int retVal = sqlite3_close(db);
		if(retVal != 0) {
			throw std::runtime_error(sqlite3_errstr(retVal));
		}
		db = nullptr;
	}
}
