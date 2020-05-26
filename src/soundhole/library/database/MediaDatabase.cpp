//
//  MediaDatabase.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaDatabase.hpp"
#include "MediaDatabaseSQL.hpp"
#include "SQLiteTransaction.hpp"
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

	Promise<std::map<String,LinkedList<Json>>> MediaDatabase::transaction(TransactionOptions options, Function<void(SQLiteTransaction&)> executor) {
		return Promise<std::map<String,LinkedList<Json>>>([=](auto resolve, auto reject) {
			if(db == nullptr || queue == nullptr) {
				reject(std::runtime_error("Cannot query an unopened database"));
				return;
			}
			auto tx = SQLiteTransaction(db, {
				.useSQLTransaction=options.useSQLTransaction
			});
			try {
				executor(tx);
			}
			catch(...) {
				reject(std::current_exception());
				return;
			}
			queue->async([=]() {
				auto exTx = std::move(tx);
				std::map<String,LinkedList<Json>> results;
				try {
					results = exTx.execute();
				} catch(...) {
					reject(std::current_exception());
					return;
				}
				resolve(results);
			});
		});
	}

	Promise<std::map<String,LinkedList<Json>>> MediaDatabase::transaction(Function<void(SQLiteTransaction&)> executor) {
		return transaction(TransactionOptions(), executor);
	}

	Promise<void> MediaDatabase::initialize(InitializeOptions options) {
		return transaction([=](auto& tx) {
			if(options.purge) {
				tx.addSQL(sql::purgeDB(), {});
			}
			tx.addSQL(sql::createDB(), {});
		}).toVoid();
	}

	Promise<void> MediaDatabase::reset() {
		return transaction([=](auto& tx) {
			tx.addSQL(sql::purgeDB(), {});
			tx.addSQL(sql::createDB(), {});
		}).toVoid();
	}
}
