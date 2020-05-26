//
//  SQLiteTransaction.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

struct sqlite3;

namespace sh {
	class SQLiteTransaction {
	public:
		struct Options {
			bool useSQLTransaction = true;
		};
		SQLiteTransaction(sqlite3* db, Options options = Options{.useSQLTransaction=true});
		
		struct AddSQLOptions {
			String outKey;
			Function<Json(Json)> mapper;
		};
		void addSQL(String sql, LinkedList<Any> params, AddSQLOptions options = AddSQLOptions());
		std::map<String,LinkedList<Json>> execute();
		
	private:
		struct ExecuteSQLOptions {
			Function<Json(Json)> mapper;
			bool waitIfBusy = false;
			bool returnResults = true;
		};
		LinkedList<Json> executeSQL(String sql, LinkedList<Any> params, ExecuteSQLOptions options = ExecuteSQLOptions{.waitIfBusy=false,.returnResults=true});
		
		struct Block {
			String sql;
			LinkedList<Any> params;
			String outKey;
			Function<Json(Json)> mapper;
		};
		
		sqlite3* db;
		Options options;
		LinkedList<Block> blocks;
	};
}
