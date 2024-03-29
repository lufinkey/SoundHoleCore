//
//  MediaDatabase.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaDatabase.hpp"
#include "MediaDatabaseSQL.hpp"
#include "MediaDatabaseSQLOperations.hpp"
#include "MediaDatabaseSQLTransformations.hpp"
#include "SQLiteTransaction.hpp"
#include <soundhole/utils/Utils.hpp>
#include <sqlite3.h>

namespace sh {
	MediaDatabase::MediaDatabase(Options options)
	: options(options), db(nullptr), queue(new DispatchQueue("MediaDatabase")) {
		//
	}

	MediaDatabase::~MediaDatabase() {
		delete queue;
		queue = nullptr;
		if(isOpen()) {
			close();
		}
	}

	MediaProviderStash* MediaDatabase::mediaProviderStash() {
		return options.mediaProviderStash;
	}

	const MediaProviderStash* MediaDatabase::mediaProviderStash() const {
		return options.mediaProviderStash;
	}

	ScrobblerStash* MediaDatabase::scrobblerStash() {
		return options.scrobblerStash;
	}

	const ScrobblerStash* MediaDatabase::scrobblerStash() const {
		return options.scrobblerStash;
	}


	#pragma mark DB operations

	void MediaDatabase::open() {
		std::unique_lock<std::recursive_mutex> lock(dbMutex);
		if(db != nullptr) {
			return;
		}
		sqlite3* tmpDb;
		int retVal = sqlite3_open(options.path.c_str(), &tmpDb);
		if(retVal != 0) {
			sqlite3_close(tmpDb);
			String errorMsg = sqlite3_errstr(retVal);
			throw std::runtime_error(errorMsg);
		}
		db = tmpDb;
	}

	bool MediaDatabase::isOpen() const {
		return (db != nullptr);
	}

	void MediaDatabase::close() {
		std::unique_lock<std::recursive_mutex> lock(dbMutex);
		if(db == nullptr) {
			return;
		}
		int retVal = sqlite3_close(db);
		if(retVal != 0) {
			throw std::runtime_error(sqlite3_errstr(retVal));
		}
		db = nullptr;
	}

	Promise<std::map<String,LinkedList<Json>>> MediaDatabase::transaction(TransactionOptions options, Function<void(SQLiteTransaction&)> executor) {
		return Promise<std::map<String,LinkedList<Json>>>([=](auto resolve, auto reject) {
			std::unique_lock<std::recursive_mutex> lock(this->dbMutex);
			if(this->db == nullptr) {
				lock.unlock();
				reject(std::runtime_error("Cannot query an unopened database"));
				return;
			}
			auto tx = SQLiteTransaction(this->db, {
				.useSQLTransaction=options.useSQLTransaction
			});
			try {
				executor(tx);
			}
			catch(...) {
				lock.unlock();
				reject(std::current_exception());
				return;
			}
			queue->async([=]() {
				std::unique_lock<std::recursive_mutex> lock(this->dbMutex);
				if(this->db == nullptr) {
					lock.unlock();
					reject(std::runtime_error("database was unexpectedly closed"));
					return;
				}
				auto exTx = std::move(tx);
				exTx.setDB(this->db);
				std::map<String,LinkedList<Json>> results;
				try {
					results = exTx.execute();
				} catch(...) {
					lock.unlock();
					reject(std::current_exception());
					return;
				}
				lock.unlock();
				resolve(results);
			});
		});
	}

	Promise<void> MediaDatabase::initialize(InitializeOptions options) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			tx.addSQL(sql::createDB(), {});
		}).toVoid();
	}

	Promise<void> MediaDatabase::reset() {
		return Promise<void>([=](auto resolve, auto reject) {
			queue->async([=]() {
				std::exception_ptr error;
				std::unique_lock<std::recursive_mutex> lock(this->dbMutex);
				// close db if needed
				bool needsToReopen = false;
				if(this->db != nullptr) {
					needsToReopen = true;
					try {
						close();
					} catch(...) {
						lock.unlock();
						reject(std::current_exception());
						return;
					}
				}
				// delete file
				try {
					fs::remove(this->options.path);
				} catch(...) {
					// reopen if needed and rethrow error
					error = std::current_exception();
					if(needsToReopen) {
						try {
							open();
						} catch(...) {
							// ignore open error
						}
					}
					lock.unlock();
					reject(error);
					return;
				}
				// re-initialize db
				sqlite3* tmpDb;
				int opnRetVal = sqlite3_open(this->options.path.c_str(), &tmpDb);
				if(opnRetVal == 0) {
					auto tx = SQLiteTransaction(tmpDb, {
						.useSQLTransaction=true
					});
					tx.addSQL(sql::createDB(), {});
					try {
						tx.execute();
					} catch(...) {
						error = std::current_exception();
					}
					sqlite3_close(tmpDb);
				} else {
					// save error and close db
					String errorMsg = sqlite3_errstr(opnRetVal);
					sqlite3_close(tmpDb);
					error = std::make_exception_ptr(std::runtime_error(errorMsg));
				}
				// re-open db
				if(needsToReopen) {
					try {
						open();
					} catch(...) {
						if(!error) {
							error = std::current_exception();
						}
					}
				}
				// resolve, or reject with error if there was one
				if(error) {
					reject(error);
				} else {
					resolve();
				}
			});
		});
	}



	#pragma mark Track

	Promise<void> MediaDatabase::cacheTracks(ArrayList<$<Track>> tracks, CacheOptions options) {
		if(tracks.size() == 0 && options.dbState.size() == 0) {
			return Promise<void>::resolve();
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceTracks(tx, tracks, true);
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<LinkedList<Json>> MediaDatabase::getTracksJson(ArrayList<String> uris) {
		if(uris.size() == 0) {
			return Promise<LinkedList<Json>>::resolve({});
		}
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			for(size_t i=0; i<uris.size(); i++) {
				sql::selectTrack(tx, std::to_string(i), uris[i]);
			}
		}).map(nullptr, [=](auto results) -> LinkedList<Json> {
			LinkedList<Json> rows;
			for(size_t i=0; i<results.size(); i++) {
				auto resultRows = results[std::to_string(i)];
				if(resultRows.size() > 0) {
					rows.pushBack(resultRows.front());
				} else {
					rows.pushBack(Json());
				}
			}
			return rows;
		});
	}

	Promise<Json> MediaDatabase::getTrackJson(String uri) {
		return getTracksJson({ uri }).map(nullptr, [=](auto rows) -> Json {
			if(rows.size() == 0 || rows.front().is_null()) {
				throw std::runtime_error("Track not found in database for uri "+uri);
			}
			return rows.front();
		});
	}

	Promise<size_t> MediaDatabase::getTrackCount() {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectTrackCount(tx, "count");
		}).map(nullptr, [=](auto results) -> size_t {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front().number_value();
		});
	}



	#pragma mark TrackCollection

	Promise<void> MediaDatabase::cacheTrackCollections(ArrayList<$<TrackCollection>> collections, CacheOptions options) {
		if(collections.size() == 0 && options.dbState.size() == 0) {
			return Promise<void>::resolve();
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceTrackCollections(tx, collections);
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<void> MediaDatabase::updateTrackCollectionVersionId($<TrackCollection> collection, CacheOptions options) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::updateTrackCollectionVersionId(tx, collection->uri(), collection->versionId());
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<LinkedList<Json>> MediaDatabase::getTrackCollectionsJson(ArrayList<String> uris) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			for(size_t i=0; i<uris.size(); i++) {
				sql::selectTrackCollectionWithOwner(tx, std::to_string(i), uris[i]);
			}
		}).map(nullptr, [=](auto results) -> LinkedList<Json> {
			LinkedList<Json> rows;
			for(size_t i=0; i<results.size(); i++) {
				auto resultRows = results[std::to_string(i)];
				if(resultRows.size() > 0) {
					rows.pushBack(resultRows.front());
				} else {
					rows.pushBack(Json());
				}
			}
			return rows;
		});
	}

	Promise<Json> MediaDatabase::getTrackCollectionJson(String uri, Optional<sql::IndexRange> itemsRange) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectTrackCollectionWithOwner(tx, "collection", uri);
			if(itemsRange) {
				sql::selectTrackCollectionItemsWithTracks(tx, "items", uri, itemsRange);
			}
		}).map(nullptr, [=](auto results) -> Json {
			auto collectionRows = results["collection"];
			if(collectionRows.size() == 0) {
				throw std::runtime_error("TrackCollection not found in database for uri "+uri);
			}
			return sql::combineDBTrackCollectionAndItems(collectionRows.front(), results["items"]);
		});
	}



	#pragma mark TrackCollectionItem

	Promise<void> MediaDatabase::cacheTrackCollectionItems($<TrackCollection> collection, Optional<sql::IndexRange> itemsRange, CacheOptions options) {
		bool includeTrackAlbums = (std::dynamic_pointer_cast<Album>(collection) == nullptr);
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceItemsFromTrackCollection(tx, collection, {
				.range = itemsRange,
				.includeTrackAlbums = includeTrackAlbums
			});
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<std::map<size_t,Json>> MediaDatabase::getTrackCollectionItemsJson(String collectionURI, sql::IndexRange range) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectTrackCollectionItemsWithTracks(tx, "items", collectionURI, range);
		}).map(nullptr, [=](auto results) -> std::map<size_t,Json> {
			std::map<size_t,Json> items;
			auto itemsJson = results["items"];
			for(auto& itemJson : itemsJson) {
				auto indexNum = itemJson["indexNum"];
				if(!indexNum.is_number()) {
					continue;
				}
				items[(size_t)indexNum.number_value()] = itemJson;
			}
			return items;
		});
	}



	#pragma mark Artist

	Promise<void> MediaDatabase::cacheArtists(ArrayList<$<Artist>> artists, CacheOptions options) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceArtists(tx, artists);
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<LinkedList<Json>> MediaDatabase::getArtistsJson(ArrayList<String> uris) {
		if(uris.size() == 0) {
			return Promise<LinkedList<Json>>::resolve({});
		}
		return transaction({.useSQLTransaction=false},[=](auto& tx) {
			for(size_t i=0; i<uris.size(); i++) {
				sql::selectArtist(tx, std::to_string(i), uris[i]);
			}
		}).map(nullptr, [=](auto results) -> LinkedList<Json> {
			LinkedList<Json> rows;
			for(size_t i=0; i<results.size(); i++) {
				auto resultRows = results[std::to_string(i)];
				if(resultRows.size() > 0) {
					rows.pushBack(resultRows.front());
				} else {
					rows.pushBack(Json());
				}
			}
			return rows;
		});
	}

	Promise<Json> MediaDatabase::getArtistJson(String uri) {
		return getArtistsJson({ uri }).map(nullptr, [=](auto rows) -> Json {
			if(rows.size() == 0 || rows.front().is_null()) {
				throw std::runtime_error("Artist not found in database for uri "+uri);
			}
			return rows.front();
		});
	}



	#pragma mark LibraryItem

	Promise<void> MediaDatabase::cacheLibraryItems(ArrayList<MediaProvider::LibraryItem> items, CacheOptions options) {
		if(items.size() == 0 && options.dbState.size() == 0) {
			return Promise<void>::resolve();
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceLibraryItems(tx, items);
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}



	#pragma mark SavedTrack

	Promise<size_t> MediaDatabase::getSavedTracksCount(GetSavedItemsCountOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedTrackCount(tx, "count", options.libraryProvider);
		}).map(nullptr, [=](auto results) -> size_t {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front().number_value();
		});
	}

	Promise<MediaDatabase::GetJsonItemsListResult> MediaDatabase::getSavedTracksJson(sql::IndexRange range, GetSavedTracksOptions options) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::selectSavedTrackCount(tx, "count", options.libraryProvider);
			sql::selectSavedTracksWithTracks(tx, "items", {
				.libraryProvider = options.libraryProvider,
				.range = range,
				.order = options.order,
				.orderBy = options.orderBy
			});
		}).map(nullptr, [=](auto results) -> GetJsonItemsListResult {
			auto countItems = results["count"];
			if(countItems.size() == 0) {
				throw std::runtime_error("failed to get items count");
			}
			size_t total = (size_t)countItems.front().number_value();
			auto rows = LinkedList<Json>(results["items"]);
			return GetJsonItemsListResult{
				.items = rows,
				.total = total
			};
		});
	}

	Promise<Json> MediaDatabase::getSavedTrackJson(String uri) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedTrackWithTrack(tx, "items", uri);
		}).map(nullptr, [=](auto results) -> Json {
			auto rows = results["items"];
			if(rows.size() == 0) {
				return nullptr;
			}
			return rows.front();
		});
	}

	Promise<ArrayList<bool>> MediaDatabase::hasSavedTracks(ArrayList<String> uris) {
		size_t uriCount = uris.size();
		if(uriCount == 0) {
			return Promise<ArrayList<bool>>::resolve({});
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			size_t i = 0;
			for(auto& uri : uris) {
				sql::selectSavedTrack(tx, std::to_string(i), uri);
				i++;
			}
		}).map(nullptr, [=](auto results) {
			ArrayList<bool> boolResults;
			boolResults.reserve(uriCount);
			for(size_t i=0; i<uriCount; i++) {
				auto& list = results[std::to_string(i)];
				boolResults.pushBack(list.size() > 0);
			}
			return boolResults;
		});
	}

	Promise<void> MediaDatabase::deleteSavedTrack(String uri) {
		return transaction({}, [=](auto& tx) {
			sql::deleteSavedTrack(tx, uri);
		}).toVoid();
	}



	#pragma mark SavedAlbum

	Promise<size_t> MediaDatabase::getSavedAlbumsCount(GetSavedItemsCountOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedAlbumCount(tx, "count", options.libraryProvider);
		}).map(nullptr, [=](auto results) -> size_t {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front().number_value();
		});
	}

	Promise<MediaDatabase::GetJsonItemsListResult> MediaDatabase::getSavedAlbumsJson(GetSavedAlbumsOptions options) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::selectSavedAlbumCount(tx, "count", options.libraryProvider);
			sql::selectSavedAlbumsWithAlbums(tx, "items", {
				.libraryProvider = options.libraryProvider,
				.range = options.range,
				.order = options.order,
				.orderBy = options.orderBy
			});
		}).map(nullptr, [=](auto results) -> GetJsonItemsListResult {
			auto countItems = results["count"];
			if(countItems.size() == 0) {
				throw std::runtime_error("failed to get items count");
			}
			size_t total = (size_t)countItems.front().number_value();
			auto rows = LinkedList<Json>(results["items"]);
			return GetJsonItemsListResult{
				.items = rows,
				.total = total
			};
		});
	}

	Promise<Json> MediaDatabase::getSavedAlbumJson(String uri) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedAlbumWithAlbum(tx, "items", uri);
		}).map(nullptr, [=](auto results) -> Json {
			auto rows = results["items"];
			if(rows.size() == 0) {
				return nullptr;
			}
			return rows.front();
		});
	}

	Promise<ArrayList<bool>> MediaDatabase::hasSavedAlbums(ArrayList<String> uris) {
		size_t uriCount = uris.size();
		if(uriCount == 0) {
			return Promise<ArrayList<bool>>::resolve({});
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			size_t i = 0;
			for(auto& uri : uris) {
				sql::selectSavedAlbum(tx, std::to_string(i), uri);
				i++;
			}
		}).map(nullptr, [=](auto results) {
			ArrayList<bool> boolResults;
			boolResults.reserve(uriCount);
			for(size_t i=0; i<uriCount; i++) {
				auto& list = results[std::to_string(i)];
				boolResults.pushBack(list.size() > 0);
			}
			return boolResults;
		});
	}

	Promise<void> MediaDatabase::deleteSavedAlbum(String uri) {
		return transaction({}, [=](auto& tx) {
			sql::deleteSavedAlbum(tx, uri);
		}).toVoid();
	}



	#pragma mark SavedPlaylist

	Promise<size_t> MediaDatabase::getSavedPlaylistsCount(GetSavedItemsCountOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedPlaylistCount(tx, "count", options.libraryProvider);
		}).map(nullptr, [=](auto results) -> size_t {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front().number_value();
		});
	}

	Promise<MediaDatabase::GetJsonItemsListResult> MediaDatabase::getSavedPlaylistsJson(GetSavedPlaylistsOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedPlaylistCount(tx, "count", options.libraryProvider);
			sql::selectSavedPlaylistsWithPlaylistsAndOwners(tx, "items", {
				.libraryProvider = options.libraryProvider,
				.range = options.range,
				.order = options.order,
				.orderBy = options.orderBy
			});
		}).map(nullptr, [=](auto results) -> GetJsonItemsListResult {
			auto countItems = results["count"];
			if(countItems.size() == 0) {
				throw std::runtime_error("failed to get items count");
			}
			size_t total = (size_t)countItems.front().number_value();
			auto rows = LinkedList<Json>(results["items"]);
			return GetJsonItemsListResult{
				.items=rows,
				.total=total
			};
		});
	}

	Promise<Json> MediaDatabase::getSavedPlaylistJson(String uri) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedPlaylistWithPlaylistAndOwner(tx, "items", uri);
		}).map(nullptr, [=](auto results) -> Json {
			auto rows = results["items"];
			if(rows.size() == 0) {
				return nullptr;
			}
			return rows.front();
		});
	}

	Promise<ArrayList<bool>> MediaDatabase::hasSavedPlaylists(ArrayList<String> uris) {
		size_t uriCount = uris.size();
		if(uriCount == 0) {
			return Promise<ArrayList<bool>>::resolve({});
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			size_t i = 0;
			for(auto& uri : uris) {
				sql::selectSavedPlaylist(tx, std::to_string(i), uri);
				i++;
			}
		}).map(nullptr, [=](auto results) {
			ArrayList<bool> boolResults;
			boolResults.reserve(uriCount);
			for(size_t i=0; i<uriCount; i++) {
				auto& list = results[std::to_string(i)];
				boolResults.pushBack(list.size() > 0);
			}
			return boolResults;
		});
	}

	Promise<void> MediaDatabase::deleteSavedPlaylist(String uri) {
		return transaction({}, [=](auto& tx) {
			sql::deleteSavedPlaylist(tx, uri);
		}).toVoid();
	}



	#pragma mark FollowedArtist

	Promise<size_t> MediaDatabase::getFollowedArtistsCount(GetSavedItemsCountOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectFollowedArtistCount(tx, "count", options.libraryProvider);
		}).map(nullptr, [=](auto results) -> size_t {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front().number_value();
		});
	}

	Promise<MediaDatabase::GetJsonItemsListResult> MediaDatabase::getFollowedArtistsJson(GetFollowedArtistsOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectFollowedArtistCount(tx, "count", options.libraryProvider);
			sql::selectFollowedArtistsWithArtists(tx, "items", {
				.libraryProvider = options.libraryProvider,
				.range = options.range,
				.order = options.order,
				.orderBy = options.orderBy
			});
		}).map(nullptr, [=](auto results) -> GetJsonItemsListResult {
			auto countItems = results["count"];
			if(countItems.size() == 0) {
				throw std::runtime_error("failed to get items count");
			}
			size_t total = (size_t)countItems.front().number_value();
			auto rows = LinkedList<Json>(results["items"]);
			return GetJsonItemsListResult{
				.items=rows,
				.total=total
			};
		});
	}

	Promise<Json> MediaDatabase::getFollowedArtistJson(String uri) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectFollowedArtistWithArtist(tx, "items", uri);
		}).map(nullptr, [=](auto results) -> Json {
			auto rows = results["items"];
			if(rows.size() == 0) {
				return nullptr;
			}
			return rows.front();
		});
	}

	Promise<ArrayList<bool>> MediaDatabase::hasFollowedArtists(ArrayList<String> uris) {
		size_t uriCount = uris.size();
		if(uriCount == 0) {
			return Promise<ArrayList<bool>>::resolve({});
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			size_t i = 0;
			for(auto& uri : uris) {
				sql::selectFollowedArtist(tx, std::to_string(i), uri);
				i++;
			}
		}).map(nullptr, [=](auto results) {
			ArrayList<bool> boolResults;
			boolResults.reserve(uriCount);
			for(size_t i=0; i<uriCount; i++) {
				auto& list = results[std::to_string(i)];
				boolResults.pushBack(list.size() > 0);
			}
			return boolResults;
		});
	}

	Promise<void> MediaDatabase::deleteFollowedArtist(String uri) {
		return transaction({}, [=](auto& tx) {
			sql::deleteFollowedArtist(tx, uri);
		}).toVoid();
	}



	#pragma mark FollowedUserAccount

	Promise<size_t> MediaDatabase::getFollowedUserAccountsCount(GetSavedItemsCountOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectFollowedUserAccountCount(tx, "count", options.libraryProvider);
		}).map(nullptr, [=](auto results) -> size_t {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front().number_value();
		});
	}

	Promise<MediaDatabase::GetJsonItemsListResult> MediaDatabase::getFollowedUserAccountsJson(GetFollowedUserAccountsOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectFollowedUserAccountCount(tx, "count", options.libraryProvider);
			sql::selectFollowedUserAccountsWithUserAccounts(tx, "items", {
				.libraryProvider = options.libraryProvider,
				.range = options.range,
				.order = options.order,
				.orderBy = options.orderBy
			});
		}).map(nullptr, [=](auto results) -> GetJsonItemsListResult {
			auto countItems = results["count"];
			if(countItems.size() == 0) {
				throw std::runtime_error("failed to get items count");
			}
			size_t total = (size_t)countItems.front().number_value();
			auto rows = LinkedList<Json>(results["items"]);
			return GetJsonItemsListResult{
				.items=rows,
				.total=total
			};
		});
	}

	Promise<Json> MediaDatabase::getFollowedUserAccountJson(String uri) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectFollowedUserAccountWithUserAccount(tx, "items", uri);
		}).map(nullptr, [=](auto results) -> Json {
			auto rows = results["items"];
			if(rows.size() == 0) {
				return nullptr;
			}
			return rows.front();
		});
	}

	Promise<ArrayList<bool>> MediaDatabase::hasFollowedUserAccounts(ArrayList<String> uris) {
		size_t uriCount = uris.size();
		if(uriCount == 0) {
			return Promise<ArrayList<bool>>::resolve({});
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			size_t i = 0;
			for(auto& uri : uris) {
				sql::selectFollowedUserAccount(tx, std::to_string(i), uri);
				i++;
			}
		}).map(nullptr, [=](auto results) {
			ArrayList<bool> boolResults;
			boolResults.reserve(uriCount);
			for(size_t i=0; i<uriCount; i++) {
				auto& list = results[std::to_string(i)];
				boolResults.pushBack(list.size() > 0);
			}
			return boolResults;
		});
	}

	Promise<void> MediaDatabase::deleteFollowedUserAccount(String uri) {
		return transaction({}, [=](auto& tx) {
			sql::deleteFollowedUserAccount(tx, uri);
		}).toVoid();
	}



	#pragma mark PlaybackHistoryItem

	Promise<void> MediaDatabase::cachePlaybackHistoryItems(ArrayList<$<PlaybackHistoryItem>> historyItems, CacheOptions options) {
		if(historyItems.size() == 0 && options.dbState.size() == 0) {
			return Promise<void>::resolve();
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplacePlaybackHistoryItems(tx, historyItems);
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<size_t> MediaDatabase::getPlaybackHistoryItemCount(PlaybackHistoryItemFilters filters) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectPlaybackHistoryItemCount(tx, "count", {
				.provider = filters.provider,
				.trackURIs = filters.trackURIs,
				.minDate = filters.minDate,
				.minDateInclusive = filters.minDateInclusive,
				.maxDate = filters.maxDate,
				.maxDateInclusive = filters.maxDateInclusive,
				.minDuration = filters.minDuration,
				.minDurationRatio = filters.minDurationRatio,
				.includeNullDuration = filters.includeNullDuration,
				.visibility = filters.visibility,
				.scrobbledBy = filters.scrobbledBy,
				.notScrobbledBy = filters.notScrobbledBy
			});
		}).map(nullptr, [=](auto results) {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front().number_value();
		});
	}

	Promise<MediaDatabase::GetJsonItemsListResult> MediaDatabase::getPlaybackHistoryItemsJson(GetPlaybackHistoryItemsOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			auto& filters = options.filters;
			auto sqlFilters = sql::PlaybackHistorySelectFilters{
				.provider = filters.provider,
				.trackURIs = filters.trackURIs,
				.minDate = filters.minDate,
				.minDateInclusive = filters.minDateInclusive,
				.maxDate = filters.maxDate,
				.maxDateInclusive = filters.maxDateInclusive,
				.minDuration = filters.minDuration,
				.minDurationRatio = filters.minDurationRatio,
				.includeNullDuration = filters.includeNullDuration,
				.visibility = filters.visibility,
				.scrobbledBy = filters.scrobbledBy,
				.notScrobbledBy = filters.notScrobbledBy
			};
			sql::selectPlaybackHistoryItemCount(tx, "count", sqlFilters);
			sql::selectPlaybackHistoryItemsWithTracks(tx, "items", sqlFilters, {
				.range = options.range,
				.order = options.order
			});
		}).map(nullptr, [=](auto results) {
			auto countItems = results["count"];
			if(countItems.size() == 0) {
				throw std::runtime_error("failed to get items count");
			}
			size_t total = (size_t)countItems.front().number_value();
			auto rows = LinkedList<Json>(results["items"]);
			return GetJsonItemsListResult{
				.items = rows,
				.total = total
			};
		});
	}

	Promise<void> MediaDatabase::deletePlaybackHistoryItem(Date startTime, String trackURI) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::deleteUnmatchedScrobbles(tx, startTime, trackURI);
			sql::deleteHistoryItemScrobbles(tx, startTime, trackURI);
			sql::deletePlaybackHistoryItem(tx, startTime, trackURI);
		}).toVoid();
	}



	#pragma mark Scrobble

	Promise<void> MediaDatabase::cacheScrobbles(ArrayList<$<Scrobble>> scrobbles, CacheOptions options) {
		if(scrobbles.size() == 0 && options.dbState.size() == 0) {
			return Promise<void>::resolve();
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceScrobbles(tx, scrobbles);
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<size_t> MediaDatabase::getScrobbleCount(ScrobbleFilters filters) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectScrobbleCount(tx, "count", {
				.scrobbler = filters.scrobbler,
				.startTimes = filters.startTimes,
				.minStartTime = filters.minStartTime,
				.minStartTimeInclusive = filters.minStartTimeInclusive,
				.maxStartTime = filters.maxStartTime,
				.maxStartTimeInclusive = filters.maxStartTimeInclusive,
				.uploaded = filters.uploaded
			});
		}).map(nullptr, [=](auto results) {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front().number_value();
		});
	}

	Promise<MediaDatabase::GetJsonItemsListResult> MediaDatabase::getScrobblesJson(GetScrobblesOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			auto& filters = options.filters;
			auto sqlFilters = sql::ScrobbleSelectFilters{
				.scrobbler = filters.scrobbler,
				.startTimes = filters.startTimes,
				.minStartTime = filters.minStartTime,
				.minStartTimeInclusive = filters.minStartTimeInclusive,
				.maxStartTime = filters.maxStartTime,
				.maxStartTimeInclusive = filters.maxStartTimeInclusive,
				.uploaded = filters.uploaded
			};
			sql::selectScrobbleCount(tx, "count", sqlFilters);
			sql::selectScrobbles(tx, "items", sqlFilters, {
				.range = options.range,
				.order = options.order
			});
		}).map(nullptr, [=](auto results) {
			auto countItems = results["count"];
			if(countItems.size() == 0) {
				throw std::runtime_error("failed to get items count");
			}
			size_t total = (size_t)countItems.front().number_value();
			auto rows = LinkedList<Json>(results["items"]);
			return GetJsonItemsListResult{
				.items = rows,
				.total = total
			};
		});
	}

	Promise<MediaDatabase::GetItemsListResult<$<Scrobble>>> MediaDatabase::getScrobbles(GetScrobblesOptions options) {
		return getScrobblesJson(options).map([=](auto results) {
			auto scrobblerStash = this->scrobblerStash();
			return GetItemsListResult<$<Scrobble>>{
				.items = results.items.map([&](auto& json) {
					return Scrobble::fromJson(json, scrobblerStash);
				}),
				.total = results.total
			};
		});
	}

	Promise<ArrayList<bool>> MediaDatabase::hasMatchingScrobbles(ArrayList<$<Scrobble>> scrobbles) {
		size_t scrobbleCount = scrobbles.size();
		if(scrobbleCount == 0) {
			return resolveWith(ArrayList<bool>());
		}
		return transaction({}, [=](auto& tx) {
			for(auto [i, scrobble] : enumerate(scrobbles)) {
				sql::selectMatchingScrobbleLocalID(tx, std::to_string(i), scrobble);
			}
		}).map(nullptr, [=](auto results) {
			ArrayList<bool> boolResults;
			boolResults.reserve(scrobbleCount);
			for(size_t i=0; i<scrobbleCount; i++) {
				auto& rows = results[std::to_string(i)];
				boolResults.pushBack(rows.size() > 0);
			}
			return boolResults;
		});
	}

	Promise<void> MediaDatabase::deleteScrobble(String localID) {
		return transaction({}, [=](auto& tx) {
			sql::deleteScrobble(tx, localID);
		}).toVoid();
	}



	#pragma mark UnmatchedScrobble

	Promise<void> MediaDatabase::cacheUnmatchedScrobbles(ArrayList<UnmatchedScrobble> scrobbles, CacheOptions options) {
		if(scrobbles.size() == 0 && options.dbState.size() == 0) {
			return Promise<void>::resolve();
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceUnmatchedScrobbles(tx, scrobbles);
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<size_t> MediaDatabase::getUnmatchedScrobbleCount(UnmatchedScrobbleFilters filters) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectUnmatchedScrobbleCount(tx, "count", {
				.scrobbler = filters.scrobbler,
				.startTimes = filters.startTimes,
				.minStartTime = filters.minStartTime,
				.minStartTimeInclusive = filters.minStartTimeInclusive,
				.maxStartTime = filters.maxStartTime,
				.maxStartTimeInclusive = filters.maxStartTimeInclusive
			});
		}).map(nullptr, [=](auto results) {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front().number_value();
		});
	}

	Promise<MediaDatabase::GetJsonItemsListResult> MediaDatabase::getUnmatchedScrobblesJson(GetUnmatchedScrobblesOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			auto& filters = options.filters;
			auto sqlSelectFilters = sql::UnmatchedScrobbleSelectFilters{
				.scrobbler = filters.scrobbler,
				.startTimes = filters.startTimes,
				.minStartTime = filters.minStartTime,
				.minStartTimeInclusive = filters.minStartTimeInclusive,
				.maxStartTime = filters.maxStartTime,
				.maxStartTimeInclusive = filters.maxStartTimeInclusive
			};
			sql::selectUnmatchedScrobbleCount(tx, "count", sqlSelectFilters);
			sql::selectUnmatchedScrobbles(tx, "items", sqlSelectFilters, {
				.range = options.range,
				.order = options.order
			});
		}).map(nullptr, [=](auto results) {
			auto countItems = results["count"];
			if(countItems.size() == 0) {
				throw std::runtime_error("failed to get items count");
			}
			size_t total = (size_t)countItems.front().number_value();
			auto rows = LinkedList<Json>(results["items"]);
			return GetJsonItemsListResult{
				.items = rows,
				.total = total
			};
		});
	}

	Promise<MediaDatabase::GetItemsListResult<UnmatchedScrobble>> MediaDatabase::getUnmatchedScrobbles(GetUnmatchedScrobblesOptions options) {
		return getUnmatchedScrobblesJson(options).map([=](auto results) {
			auto providerStash = this->mediaProviderStash();
			auto scrobblerStash = this->scrobblerStash();
			return GetItemsListResult<UnmatchedScrobble>{
				.items = results.items.map([&](auto& json) {
					return UnmatchedScrobble::fromJson(json, providerStash, scrobblerStash);
				}),
				.total = results.total
			};
		});
	}

	Promise<void> MediaDatabase::replaceUnmatchedScrobbles(ArrayList<Tuple<UnmatchedScrobble, $<Scrobble>>> scrobbleTuples, CacheOptions options) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			// delete all unmatched scrobbles
			for(auto& scrobbleTuple : scrobbleTuples) {
				auto& unmatchedScrobble = std::get<UnmatchedScrobble>(scrobbleTuple);
				sql::deleteUnmatchedScrobble(tx,
					unmatchedScrobble.scrobbler->name(),
					unmatchedScrobble.historyItem->startTime(),
					unmatchedScrobble.historyItem->track()->uri());
			}
			// insert new scrobbles
			sql::insertOrReplaceScrobbles(tx, scrobbleTuples.map([](auto& tuple) {
				return std::get<$<Scrobble>>(tuple);
			}));
			// apply DB state
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<void> MediaDatabase::deleteUnmatchedScrobbles(ArrayList<UnmatchedScrobble> scrobbles, CacheOptions options) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			for(auto& scrobble : scrobbles) {
				sql::deleteUnmatchedScrobble(tx,
					scrobble.scrobbler->name(),
					scrobble.historyItem->startTime(),
					scrobble.historyItem->track()->uri());
			}
			sql::applyDBState(tx, options.dbState);
		}).toVoid();
	}



	#pragma mark AlbumItem

	Promise<void> MediaDatabase::loadAlbumItems($<Album> album, Album::Mutator* mutator, size_t index, size_t count) {
		return getTrackCollectionItemsJson(album->uri(), {
			.startIndex=index,
			.endIndex=(index+count)
		}).then([=](std::map<size_t,Json> items) {
			mutator->lock([&]() {
				LinkedList<$<AlbumItem>> albumItems;
				size_t albumItemsStartIndex = index;
				size_t nextIndex = index;
				for(auto& pair : items) {
					if(pair.first != nextIndex && albumItems.size() > 0) {
						mutator->apply(albumItemsStartIndex, albumItems);
						albumItems.clear();
					}
					if(albumItems.size() == 0) {
						albumItemsStartIndex = pair.first;
					}
					albumItems.pushBack(std::static_pointer_cast<AlbumItem>(album->createCollectionItem(pair.second, options.mediaProviderStash)));
					nextIndex = pair.first + 1;
				}
				if(albumItems.size() > 0) {
					mutator->apply(albumItemsStartIndex, albumItems);
				}
			});
		});
	}

	#pragma mark PlaylistItem

	Promise<void> MediaDatabase::loadPlaylistItems($<Playlist> playlist, Playlist::Mutator* mutator, size_t index, size_t count) {
		return getTrackCollectionItemsJson(playlist->uri(), {
			.startIndex=index,
			.endIndex=(index+count)
		}).then([=](std::map<size_t,Json> items) {
			mutator->lock([&]() {
				LinkedList<$<PlaylistItem>> playlistItems;
				size_t playlistItemsStartIndex = index;
				size_t nextIndex = index;
				for(auto& pair : items) {
					if(pair.first != nextIndex && playlistItems.size() > 0) {
						mutator->apply(playlistItemsStartIndex, playlistItems);
						playlistItems.clear();
					}
					if(playlistItems.size() == 0) {
						playlistItemsStartIndex = pair.first;
					}
					playlistItems.pushBack(std::static_pointer_cast<PlaylistItem>(playlist->createCollectionItem(pair.second, options.mediaProviderStash)));
					nextIndex = pair.first + 1;
				}
				if(playlistItems.size() > 0) {
					mutator->apply(playlistItemsStartIndex, playlistItems);
				}
			});
		});
	}



	#pragma mark DBState

	Promise<void> MediaDatabase::setState(std::map<String,String> state) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::applyDBState(tx, state);
		}).toVoid();
	}

	Promise<std::map<String,String>> MediaDatabase::getState(ArrayList<String> keys) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			for(auto& key : keys) {
				sql::selectDBState(tx, key, key);
			}
		}).map(nullptr, [=](auto results) -> std::map<String,String> {
			std::map<String,String> state;
			for(auto& key : keys) {
				auto it = results.find(key);
				if(it != results.end() && it->second.size() > 0) {
					auto row = it->second.front();
					auto value = row["stateValue"];
					if(value.is_string()) {
						state[key] = value.string_value();
					}
				}
			}
			return state;
		});
	}

	Promise<String> MediaDatabase::getStateValue(String key, String defaultValue) {
		return getState({ key }).map(nullptr, [=](auto state) -> String {
			auto it = state.find(key);
			if(it == state.end()) {
				return defaultValue;
			}
			return it->second;
		});
	}


	String MediaDatabase::stateKey_syncLibraryResumeData(const MediaProvider* provider) const {
		return "syncLibraryResumeData_"+provider->name();
	}

	String MediaDatabase::stateKey_scrobblerMatchHistoryDate(const Scrobbler* scrobbler) const {
		return "scrobblerMatchHistoryDate_"+scrobbler->name();
	}
}
