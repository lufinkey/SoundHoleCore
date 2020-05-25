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
		SQLiteTransaction(sqlite3* db);
		
		void addSQL(String sql, LinkedList<Any> params, String resultKey = String());
		std::map<String,LinkedList<Json>> execute();
		
	private:
		struct ExecuteSQLOptions {
			bool waitIfBusy = false;
		};
		LinkedList<Json> executeSQL(String sql, LinkedList<Any> params, ExecuteSQLOptions options = ExecuteSQLOptions{.waitIfBusy=false});
		
		struct Block {
			String sql;
			LinkedList<Any> params;
			String outKey;
		};
		
		sqlite3* db;
		LinkedList<Block> blocks;
	};
}
