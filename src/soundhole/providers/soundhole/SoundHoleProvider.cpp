//
//  SoundHoleProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/27/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SoundHoleProvider.hpp"

namespace sh {
	SoundHoleProvider::SoundHoleProvider(Options options) {
		if(options.googledrive) {
			auto googledrive = new GoogleDriveStorageProvider(options.googledrive.value());
			storageProviders.pushBack(googledrive);
		}
	}

	SoundHoleProvider::~SoundHoleProvider() {
		for(auto storageProvider : storageProviders) {
			delete storageProvider;
		}
	}



	String SoundHoleProvider::name() const {
		return "soundhole";
	}

	String SoundHoleProvider::displayName() const {
		return "SoundHole";
	}



	#pragma mark URI/ID parsing

	SoundHoleProvider::URI SoundHoleProvider::parseURI(String uri) const {
		if(uri.empty()) {
			throw std::invalid_argument("Empty string is not a valid SoundHole uri");
		}
		auto uriParts = ArrayList<String>(uri.split(':'));
		if(uriParts.size() != 4) {
			throw std::invalid_argument("Invalid SoundHole uri "+uri);
		}
		if(uriParts[0] != name()) {
			throw std::invalid_argument("URI prefix "+uriParts[0]+" does not match provider name "+name());
		}
		return URI{
			.provider=uriParts[0],
			.storageProvider=uriParts[1],
			.type=uriParts[2],
			.id=uriParts[3]
		};
	}

	String SoundHoleProvider::createURI(String storageProvider, String type, String id) const {
		return name()+":"+storageProvider+":"+type+":"+id;
	}

	SoundHoleProvider::UserID SoundHoleProvider::parseUserID(String userId) const {
		if(userId.empty()) {
			throw std::invalid_argument("Empty string is not a valid SoundHole userId");
		}
		auto index = userId.indexOf(':');
		if(index == String::npos || index == (userId.length()-1)) {
			throw std::invalid_argument("Invalid SoundHole userId "+userId);
		}
		return UserID{
			.storageProvider=userId.substring(0, index),
			.id=userId.substring(index+1)
		};
	}



	#pragma mark Storage Providers

	StorageProvider* SoundHoleProvider::primaryStorageProvider() {
		return getStorageProvider(primaryStorageProviderName);
	}
	const StorageProvider* SoundHoleProvider::primaryStorageProvider() const {
		return getStorageProvider(primaryStorageProviderName);
	}

	StorageProvider* SoundHoleProvider::getStorageProvider(const String& name) {
		for(auto storageProvider : storageProviders) {
			if(storageProvider->name() == name) {
				return storageProvider;
			}
		}
		return nullptr;
	}
	const StorageProvider* SoundHoleProvider::getStorageProvider(const String& name) const {
		for(auto storageProvider : storageProviders) {
			if(storageProvider->name() == name) {
				return storageProvider;
			}
		}
		return nullptr;
	}



	#pragma mark Login

	Promise<bool> SoundHoleProvider::login() {
		auto storageProvider = primaryStorageProvider();
		if(storageProvider == nullptr) {
			return Promise<bool>::reject(std::runtime_error("missing primary storage provider"));
		}
		return storageProvider->login();
	}

	void SoundHoleProvider::logout() {
		auto storageProvider = primaryStorageProvider();
		if(storageProvider == nullptr) {
			throw std::runtime_error("missing primary storage provider");
		}
		storageProvider->logout();
	}

	bool SoundHoleProvider::isLoggedIn() const {
		auto storageProvider = primaryStorageProvider();
		if(storageProvider == nullptr) {
			throw std::runtime_error("missing primary storage provider");
		}
		return storageProvider->isLoggedIn();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> SoundHoleProvider::getCurrentUserIds() {
		auto promises = ArrayList<Promise<ArrayList<String>>>();
		promises.reserve(storageProviders.size());
		for(auto storageProvider : storageProviders) {
			auto providerName = storageProvider->name();
			promises.pushBack(storageProvider->getCurrentUserIds().map<ArrayList<String>>([=](auto userIds) {
				return userIds.template map<String>([&](auto& userId) {
					return createUserID(providerName, userId);
				});
			}));
		}
		return Promise<ArrayList<String>>::all(promises).map<ArrayList<String>>([=](auto userIdLists) {
			auto userIds = ArrayList<String>();
			size_t count = 0;
			for(auto& list : userIdLists) {
				count += list.size();
			}
			userIds.reserve(count);
			for(auto& list : userIdLists) {
				userIds.pushBackList(std::move(list));
			}
			return userIds;
		});
	}



	#pragma mark Media Item Fetching

	Promise<Track::Data> SoundHoleProvider::getTrackData(String uri) {
		return Promise<Track::Data>::reject(std::logic_error("SoundHoleProvider::getTrackData is unimplemented"));
	}

	Promise<Artist::Data> SoundHoleProvider::getArtistData(String uri) {
		return Promise<Artist::Data>::reject(std::logic_error("SoundHole provider does not have artists"));
	}

	Promise<Album::Data> SoundHoleProvider::getAlbumData(String uri) {
		return Promise<Album::Data>::reject(std::logic_error("SoundHole provider does not have albums"));
	}

	Promise<Playlist::Data> SoundHoleProvider::getPlaylistData(String uri) {
		return Promise<Playlist::Data>::reject(std::logic_error("SoundHoleProvider::getPlaylistData is unimplemented"));
	}

	Promise<UserAccount::Data> SoundHoleProvider::getUserData(String uri) {
		return Promise<UserAccount::Data>::reject(std::logic_error("SoundHoleProvider::getUserData is unimplemented"));
	}



	Promise<ArrayList<$<Track>>> SoundHoleProvider::getArtistTopTracks(String artistURI) {
		return Promise<ArrayList<$<Track>>>::reject(std::logic_error("SoundHole doesn't have artist top tracks"));
	}

	SoundHoleProvider::ArtistAlbumsGenerator SoundHoleProvider::getArtistAlbums(String artistURI) {
		using YieldResult = typename ArtistAlbumsGenerator::YieldResult;
		return ArtistAlbumsGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("SoundHole does not support artist albums"));
		});
	}

	SoundHoleProvider::UserPlaylistsGenerator SoundHoleProvider::getUserPlaylists(String userURI) {
		using YieldResult = typename UserPlaylistsGenerator::YieldResult;
		return UserPlaylistsGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("SoundHoleProvider::getUserPlaylists is unimplemented"));
		});
	}



	Album::MutatorDelegate* SoundHoleProvider::createAlbumMutatorDelegate($<Album> album) {
		throw std::logic_error("SoundHole does not support albums");
	}

	Playlist::MutatorDelegate* SoundHoleProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		throw std::logic_error("SoundHoleProvider::createPlaylistMutatorDelegate is unimplemented");
	}



	#pragma mark User Library

	bool SoundHoleProvider::hasLibrary() const {
		return false;
	}

	SoundHoleProvider::LibraryItemGenerator SoundHoleProvider::generateLibrary(GenerateLibraryOptions options) {
		using YieldResult = typename LibraryItemGenerator::YieldResult;
		return LibraryItemGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("SoundHoleProvider::generateLibrary is unimplemented"));
		});
	}



	#pragma mark Playlists

	bool SoundHoleProvider::canCreatePlaylists() const {
		return true;
	}

	ArrayList<Playlist::Privacy> SoundHoleProvider::supportedPlaylistPrivacies() const {
		return { Playlist::Privacy::PRIVATE, Playlist::Privacy::PUBLIC };
	}

	Promise<$<Playlist>> SoundHoleProvider::createPlaylist(String name, CreatePlaylistOptions options) {
		return Promise<$<Playlist>>::reject(std::logic_error("SoundHoleProvider::createPlaylist is unimplemented"));
	}

	Promise<bool> SoundHoleProvider::isPlaylistEditable($<Playlist> playlist) {
		return Promise<bool>::reject(std::logic_error("SoundHoleProvider::isPlaylistEditable is unimplemented"));
	}



	#pragma mark Player

	MediaPlaybackProvider* SoundHoleProvider::player() {
		return nullptr;
	}

	const MediaPlaybackProvider* SoundHoleProvider::player() const {
		return nullptr;
	}
}
