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

	Promise<void> MediaDatabase::initialize(InitializeOptions options) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			if(options.purge) {
				tx.addSQL(sql::purgeDB(), {});
			}
			tx.addSQL(sql::createDB(), {});
		}).toVoid();
	}

	Promise<void> MediaDatabase::reset() {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			tx.addSQL(sql::purgeDB(), {});
			tx.addSQL(sql::createDB(), {});
		}).toVoid();
	}



	Promise<void> MediaDatabase::cacheTracks(ArrayList<$<Track>> tracks, CacheOptions options) {
		if(tracks.size() == 0 && options.dbState.size() == 0) {
			return Promise<void>::resolve();
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceTracks(tx, tracks, true);
			applyDBState(tx, options.dbState);
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
		}).map<LinkedList<Json>>(nullptr, [=](auto results) {
			LinkedList<Json> rows;
			for(size_t i=0; i<results.size(); i++) {
				auto resultRows = results[std::to_string(i)];
				if(resultRows.size() > 0) {
					rows.pushBack(transformDBTrack(resultRows.front()));
				} else {
					rows.pushBack(Json());
				}
			}
			return rows;
		});
	}

	Promise<Json> MediaDatabase::getTrackJson(String uri) {
		return getTracksJson({ uri }).map<Json>(nullptr, [=](auto rows) {
			if(rows.size() == 0 || rows.front().is_null()) {
				throw std::runtime_error("Track not found in database for uri "+uri);
			}
			return rows.front();
		});
	}

	Promise<size_t> MediaDatabase::getTrackCount() {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectTrackCount(tx, "count");
		}).map<size_t>(nullptr, [=](auto results) {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front()["total"].number_value();
		});
	}



	Promise<void> MediaDatabase::cacheTrackCollections(ArrayList<$<TrackCollection>> collections, CacheOptions options) {
		if(collections.size() == 0 && options.dbState.size() == 0) {
			return Promise<void>::resolve();
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceTrackCollections(tx, collections);
			applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<LinkedList<Json>> MediaDatabase::getTrackCollectionsJson(ArrayList<String> uris) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			for(size_t i=0; i<uris.size(); i++) {
				sql::selectTrackCollection(tx, std::to_string(i), uris[i]);
			}
		}).map<LinkedList<Json>>(nullptr, [=](auto results) {
			LinkedList<Json> rows;
			for(size_t i=0; i<results.size(); i++) {
				auto resultRows = results[std::to_string(i)];
				if(resultRows.size() > 0) {
					rows.pushBack(transformDBTrackCollection(resultRows.front()));
				} else {
					rows.pushBack(Json());
				}
			}
			return rows;
		});
	}

	Promise<Json> MediaDatabase::getTrackCollectionJson(String uri, Optional<sql::IndexRange> itemsRange) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectTrackCollection(tx, "collection", uri);
			if(itemsRange) {
				sql::selectTrackCollectionItemsAndTracks(tx, "items", uri, itemsRange);
			}
		}).map<Json>(nullptr, [=](auto results) {
			auto collectionRows = results["collection"];
			if(collectionRows.size() == 0) {
				throw std::runtime_error("TrackCollection not found in database for uri "+uri);
			}
			return transformDBTrackCollectionAndItemsAndTracks(collectionRows.front(), results["items"]);
		});
	}



	Promise<void> MediaDatabase::cacheTrackCollectionItems($<TrackCollection> collection, Optional<sql::IndexRange> itemsRange, CacheOptions options) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceItemsFromTrackCollection(tx, collection, itemsRange);
			applyDBState(tx, options.dbState);
		}).toVoid();
	}

	Promise<std::map<size_t,Json>> MediaDatabase::getTrackCollectionItemsJson(String collectionURI, sql::IndexRange range) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectTrackCollectionItemsAndTracks(tx, "items", collectionURI, range);
		}).map<std::map<size_t,Json>>(nullptr, [=](auto results) {
			std::map<size_t,Json> items;
			auto itemsJson = results["items"];
			for(auto& json : itemsJson) {
				auto itemJson = transformDBTrackCollectionItemAndTrack(json);
				auto indexNum = itemJson["indexNum"];
				if(!indexNum.is_number()) {
					continue;
				}
				items[(size_t)indexNum.number_value()] = itemJson;
			}
			return items;
		});
	}



	Promise<void> MediaDatabase::cacheArtists(ArrayList<$<Artist>> artists, CacheOptions options) {
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceArtists(tx, artists);
			applyDBState(tx, options.dbState);
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
		}).map<LinkedList<Json>>(nullptr, [=](auto results) {
			LinkedList<Json> rows;
			for(size_t i=0; i<results.size(); i++) {
				auto resultRows = results[std::to_string(i)];
				if(resultRows.size() > 0) {
					rows.pushBack(transformDBArtist(resultRows.front()));
				} else {
					rows.pushBack(Json());
				}
			}
			return rows;
		});
	}

	Promise<Json> MediaDatabase::getArtistJson(String uri) {
		return getArtistsJson({ uri }).map<Json>(nullptr, [=](auto rows) {
			if(rows.size() == 0 || rows.front().is_null()) {
				throw std::runtime_error("Artist not found in database for uri "+uri);
			}
			return rows.front();
		});
	}



	Promise<void> MediaDatabase::cacheLibraryItems(ArrayList<MediaProvider::LibraryItem> items, CacheOptions options) {
		if(items.size() == 0 && options.dbState.size() == 0) {
			return Promise<void>::resolve();
		}
		return transaction({.useSQLTransaction=true}, [=](auto& tx) {
			sql::insertOrReplaceLibraryItems(tx, items);
			applyDBState(tx, options.dbState);
		}).toVoid();
	}


	
	Promise<size_t> MediaDatabase::getSavedTracksCount(GetSavedTracksCountOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedTrackCount(tx, "count", options.libraryProvider);
		}).map<size_t>(nullptr, [=](auto results) {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front()["total"].number_value();
		});
	}

	Promise<LinkedList<Json>> MediaDatabase::getSavedTracksJson(sql::IndexRange range, GetSavedTracksJsonOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedTracksAndTracks(tx, "items", {
				.libraryProvider=options.libraryProvider,
				.range=range
			});
		}).map<LinkedList<Json>>(nullptr, [=](auto results) {
			LinkedList<Json> rows;
			auto itemsJson = results["items"];
			for(auto& json : itemsJson) {
				auto jsonArray = json.array_items();
				if(jsonArray.size() < 2) {
					continue;
				}
				auto savedTrackJson = jsonArray[0];
				auto trackJson = jsonArray[1];
				rows.pushBack(transformDBSavedTrack(savedTrackJson, trackJson));
			}
			return rows;
		});
	}

	Promise<size_t> MediaDatabase::getSavedAlbumsCount(GetSavedAlbumsCountOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedAlbumCount(tx, "count", options.libraryProvider);
		}).map<size_t>(nullptr, [=](auto results) {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front()["total"].number_value();
		});
	}

	Promise<LinkedList<Json>> MediaDatabase::getSavedAlbumsJson(sql::IndexRange range, GetSavedAlbumsJsonOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedAlbumsAndAlbums(tx, "items", {
				.libraryProvider=options.libraryProvider,
				.range=range
			});
		}).map<LinkedList<Json>>(nullptr, [=](auto results) {
			LinkedList<Json> rows;
			auto itemsJson = results["items"];
			for(auto& json : itemsJson) {
				auto jsonArray = json.array_items();
				if(jsonArray.size() < 2) {
					continue;
				}
				auto savedAlbumJson = jsonArray[0];
				auto albumJson = jsonArray[1];
				rows.pushBack(transformDBSavedAlbum(savedAlbumJson, albumJson));
			}
			return rows;
		});
	}

	Promise<size_t> MediaDatabase::getSavedPlaylistsCount(GetSavedPlaylistsCountOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedPlaylistCount(tx, "count", options.libraryProvider);
		}).map<size_t>(nullptr, [=](auto results) {
			auto items = results["count"];
			if(items.size() == 0) {
				return (size_t)0;
			}
			return (size_t)items.front()["total"].number_value();
		});
	}

	Promise<LinkedList<Json>> MediaDatabase::getSavedPlaylistsJson(sql::IndexRange range, GetSavedPlaylistsJsonOptions options) {
		return transaction({.useSQLTransaction=false}, [=](auto& tx) {
			sql::selectSavedPlaylistsAndPlaylists(tx, "items", {
				.libraryProvider=options.libraryProvider,
				.range=range
			});
		}).map<LinkedList<Json>>(nullptr, [=](auto results) {
			LinkedList<Json> rows;
			auto itemsJson = results["items"];
			for(auto& json : itemsJson) {
				auto jsonArray = json.array_items();
				if(jsonArray.size() < 2) {
					continue;
				}
				auto savedPlaylistJson = jsonArray[0];
				auto playlistJson = jsonArray[1];
				rows.pushBack(transformDBSavedPlaylist(savedPlaylistJson, playlistJson));
			}
			return rows;
		});
	}



	void MediaDatabase::applyDBState(SQLiteTransaction& tx, std::map<std::string,std::string> state) {
		if(state.size() == 0) {
			return;
		}
		ArrayList<sql::DBState> states;
		states.reserve(state.size());
		for(auto& pair : state) {
			states.pushBack({
				.stateKey=pair.first,
				.stateValue=pair.second
			});
		}
		sql::insertOrReplaceDBStates(tx, states);
	}
	
	Json MediaDatabase::transformDBTrack(Json json) {
		auto obj = json.object_items();
		if(obj.find("type") == obj.end()) {
			obj["type"] = "track";
		}
		auto artistsJson = obj["artists"];
		if(artistsJson.is_string()) {
			std::string error;
			obj["artists"] = Json::parse(artistsJson.string_value(), error);
			if(!error.empty()) {
				throw std::invalid_argument("Failed to parse artists json: "+error);
			}
		}
		auto imagesJson = obj["images"];
		if(imagesJson.is_string()) {
			std::string error;
			obj["images"] = Json::parse(imagesJson.string_value(), error);
			if(!error.empty()) {
				throw std::invalid_argument("Failed to parse images json: "+error);
			}
		}
		return obj;
	}

	Json MediaDatabase::transformDBTrackCollection(Json json) {
		auto obj = json.object_items();
		auto artistsJson = obj["artists"];
		if(artistsJson.is_string()) {
			std::string error;
			obj["artists"] = Json::parse(artistsJson.string_value(), error);
			if(!error.empty()) {
				throw std::invalid_argument("Failed to parse artists json: "+error);
			}
		}
		auto imagesJson = obj["images"];
		if(imagesJson.is_string()) {
			std::string error;
			obj["images"] = Json::parse(imagesJson.string_value(), error);
			if(!error.empty()) {
				throw std::invalid_argument("Failed to parse images json: "+error);
			}
		}
		return obj;
	}

	Json MediaDatabase::transformDBTrackCollectionAndItemsAndTracks(Json collectionJson, LinkedList<Json> itemsAndTracksJson) {
		auto obj = transformDBTrackCollection(collectionJson).object_items();
		Json::object itemsJson;
		for(auto& itemAndTrackJson : itemsAndTracksJson) {
			auto itemJson = transformDBTrackCollectionItemAndTrack(itemAndTrackJson);
			auto indexNum = itemJson["indexNum"];
			if(!indexNum.is_number()) {
				continue;
			}
			itemsJson[std::to_string((size_t)indexNum.number_value())] = itemJson;
		}
		obj["items"] = itemsJson;
		return obj;
	}

	Json MediaDatabase::transformDBTrackCollectionItem(Json collectionItemJson, Json trackJson) {
		auto obj = collectionItemJson.object_items();
		obj["track"] = transformDBTrack(trackJson);
		return obj;
	}

	Json MediaDatabase::transformDBTrackCollectionItemAndTrack(Json itemAndTrackJson) {
		auto jsonArray = itemAndTrackJson.array_items();
		if(jsonArray.size() < 2) {
			throw std::runtime_error("invalid TrackCollectionItem and Track json object");
		}
		auto collectionItemJson = jsonArray[0];
		auto trackJson = jsonArray[1];
		return transformDBTrackCollectionItem(collectionItemJson, trackJson);
	}

	Json MediaDatabase::transformDBArtist(Json json) {
		auto obj = json.object_items();
		auto imagesJson = obj["images"];
		if(imagesJson.is_string()) {
			std::string error;
			obj["images"] = Json::parse(imagesJson.string_value(), error);
			if(!error.empty()) {
				throw std::invalid_argument("Failed to parse images json: "+error);
			}
		}
		return obj;
	}

	Json MediaDatabase::transformDBSavedTrack(Json savedTrackJson, Json trackJson) {
		auto obj = savedTrackJson.object_items();
		obj["mediaItem"] = transformDBTrack(trackJson);
		return obj;
	}

	Json MediaDatabase::transformDBSavedAlbum(Json savedAlbumJson, Json albumJson) {
		auto obj = savedAlbumJson.object_items();
		obj["mediaItem"] = transformDBTrackCollection(albumJson);
		return obj;
	}

	Json MediaDatabase::transformDBSavedPlaylist(Json savedPlaylistJson, Json playlistJson) {
		auto obj = savedPlaylistJson.object_items();
		obj["mediaItem"] = transformDBTrackCollection(playlistJson);
		return obj;
	}
}
