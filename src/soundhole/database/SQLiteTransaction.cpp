//
//  SQLiteTransaction.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SQLiteTransaction.hpp"
#include <thread>
#include <sqlite3.h>

namespace sh {
	SQLiteTransaction::SQLiteTransaction(sqlite3* db, Options options)
	: db(db), options(options) {
		//
	}

	void SQLiteTransaction::addSQL(String sql, LinkedList<Any> params, AddSQLOptions options) {
		blocks.pushBack({
			.sql=sql,
			.params=params,
			.outKey=options.outKey,
			.mapper=options.mapper
		});
	}

	std::map<String,LinkedList<Json>> SQLiteTransaction::execute() {
		if(options.useSQLTransaction) {
			executeSQL("BEGIN TRANSACTION;", {});
		}
		std::map<String,LinkedList<Json>> results;
		try {
			for(auto& block : blocks) {
				auto blockResults = executeSQL(block.sql, block.params, {
					.mapper=block.mapper,
					.returnResults=(!block.outKey.empty())
				});
				if(!block.outKey.empty()) {
					results[block.outKey] = blockResults;
				}
			}
			if(options.useSQLTransaction) {
				executeSQL("END TRANSACTION;", {}, {.waitIfBusy=true});
			}
		}
		catch(...) {
			auto error = std::current_exception();
			if(options.useSQLTransaction) {
				try {
					executeSQL("ROLLBACK TRANSACTION;", {});
				} catch(std::exception& e) {
					FGL_WARN((String)"Error while rolling back transaction: "+e.what());
				}
			}
			std::rethrow_exception(error);
		}
		return results;
	}

	LinkedList<Json> SQLiteTransaction::executeSQL(String sql, LinkedList<Any> params, ExecuteSQLOptions options) {
		LinkedList<Json> rows;
		sql = sql.trim();
		const char* nextSQL = sql.c_str();
		while(nextSQL != nullptr && nextSQL != (sql.c_str()+sql.length())) {
			// prepare statement
			sqlite3_stmt* stmt = nullptr;
			int retVal = sqlite3_prepare_v2(db, nextSQL, (int)sql.length(), &stmt, &nextSQL);
			if(retVal != SQLITE_OK) {
				String errorMsg = sqlite3_errmsg(db);
				if(stmt != nullptr) {
					sqlite3_finalize(stmt);
				}
				throw std::runtime_error("Failed to prepare SQL statement: "+errorMsg);
			}
			// bind parameters
			size_t stmtParamsCount = sqlite3_bind_parameter_count(stmt);
			if(params.size() < stmtParamsCount) {
				sqlite3_finalize(stmt);
				throw std::runtime_error((String)"Not enough parameters ("+params.size()+") for statement with "+stmtParamsCount+" parameters");
			}
			auto stmtParams = params.extractListFront(stmtParamsCount);
			size_t i=0;
			for(auto& param : stmtParams) {
				retVal = SQLITE_OK;
				if(param.empty()) {
					retVal = sqlite3_bind_null(stmt, (int)i);
				}
				else if(param.is<String>()) {
					auto& str = param.as<String>();
					retVal = sqlite3_bind_text(stmt, (int)i, str.c_str(), (int)str.length(), NULL);
				}
				else if(param.is<std::string>()) {
					auto& str = param.as<std::string>();
					retVal = sqlite3_bind_text(stmt, (int)i, str.c_str(), (int)str.length(), NULL);
				}
				else if(param.is<int>()) {
					retVal = sqlite3_bind_int(stmt, (int)i, param.as<int>());
				}
				else if(param.is<size_t>()) {
					retVal = sqlite3_bind_int64(stmt, (int)i, (sqlite3_int64)param.as<size_t>());
				}
				else if(param.is<double>()) {
					retVal = sqlite3_bind_double(stmt, (int)i, param.as<double>());
				}
				else if(param.is<float>()) {
					retVal = sqlite3_bind_double(stmt, (int)i, (double)param.as<float>());
				}
				else if(param.is<long>()) {
					retVal = sqlite3_bind_int64(stmt, (int)i, (sqlite3_int64)param.as<long>());
				}
				else if(param.is<unsigned long>()) {
					retVal = sqlite3_bind_int64(stmt, (int)i, (sqlite3_int64)param.as<unsigned long>());
				}
				else if(param.is<bool>()) {
					retVal = sqlite3_bind_int(stmt, (int)i, (int)param.as<bool>());
				}
				else {
					sqlite3_finalize(stmt);
					throw std::runtime_error((String)"Invalid SQL type "+param.typeName()+" at index "+i);
				}
				if(retVal != SQLITE_OK) {
					sqlite3_finalize(stmt);
					throw std::runtime_error((String)"Failed to bind SQL parameter \""+param.toString()+"\" at index "+i+": "+sqlite3_errstr(retVal));
				}
				i++;
			}
			// execute statement
			while(true) {
				retVal = sqlite3_step(stmt);
				if(retVal == SQLITE_ROW) {
					if(options.returnResults) {
						Json::object row;
						int colCount = sqlite3_column_count(stmt);
						for(int i=0; i<colCount; i++) {
							String columnName = sqlite3_column_name(stmt, i);
							auto sqlType = sqlite3_column_type(stmt, i);
							switch(sqlType) {
								case SQLITE_NULL: {
									row[columnName] = Json();
								} break;
								case SQLITE_INTEGER: {
									row[columnName] = (double)sqlite3_column_int64(stmt, i);
								} break;
								case SQLITE_FLOAT: {
									row[columnName] = sqlite3_column_double(stmt, i);
								} break;
								case SQLITE_TEXT: {
									auto text = sqlite3_column_text(stmt, i);
									row[columnName] = (text != nullptr) ? Json(String(text)) : Json();
								} break;
								case SQLITE_BLOB: {
									auto blob = sqlite3_column_blob(stmt, i);
									auto bytes = sqlite3_column_bytes(stmt, i);
									row[columnName] = Json(String((const char*)blob, (size_t)bytes));
								} break;
								default: {
									sqlite3_finalize(stmt);
									throw std::runtime_error((String)"Invalid sql type "+sqlType+" for column \""+columnName+"\"");
								} break;
							}
						}
						if(options.mapper) {
							rows.pushBack(options.mapper(row));
						} else {
							rows.pushBack(row);
						}
					}
				}
				else if(retVal == SQLITE_DONE) {
					sqlite3_finalize(stmt);
					break;
				}
				else if(retVal == SQLITE_BUSY && options.waitIfBusy) {
					std::this_thread::sleep_for(std::chrono::milliseconds(2));
					continue;
				}
				else {
					sqlite3_finalize(stmt);
					throw std::runtime_error((String)"Failed to step SQL statement: "+sqlite3_errstr(retVal));
				}
			}
		}
		return rows;
	}
}
