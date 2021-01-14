//
//  SoundHoleMediaProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/27/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SoundHoleMediaProvider.hpp"

namespace sh {
	SoundHoleMediaProvider::SoundHoleMediaProvider(Options options)
	: sessionPersistKey(options.sessionPersistKey) {
		if(options.googledrive) {
			auto googledrive = new GoogleDriveStorageProvider(options.googledrive.value());
			storageProviders.pushBack(googledrive);
		}
		loadUserPrefs();
	}

	SoundHoleMediaProvider::~SoundHoleMediaProvider() {
		for(auto storageProvider : storageProviders) {
			delete storageProvider;
		}
	}



	String SoundHoleMediaProvider::name() const {
		return "soundhole";
	}

	String SoundHoleMediaProvider::displayName() const {
		return "SoundHole";
	}



	#pragma mark URI/ID parsing

	SoundHoleMediaProvider::URI SoundHoleMediaProvider::parseURI(String uri) const {
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

	String SoundHoleMediaProvider::createURI(String storageProvider, String type, String id) const {
		return name()+":"+storageProvider+":"+type+":"+id;
	}

	SoundHoleMediaProvider::UserID SoundHoleMediaProvider::parseUserID(String userId) const {
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

	String SoundHoleMediaProvider::createUserID(String storageProvider, String id) const {
		return storageProvider+":"+id;
	}



	#pragma mark Storage Providers

	StorageProvider* SoundHoleMediaProvider::primaryStorageProvider() {
		return getStorageProvider(primaryStorageProviderName);
	}
	const StorageProvider* SoundHoleMediaProvider::primaryStorageProvider() const {
		return getStorageProvider(primaryStorageProviderName);
	}

	StorageProvider* SoundHoleMediaProvider::getStorageProvider(const String& name) {
		for(auto storageProvider : storageProviders) {
			if(storageProvider->name() == name) {
				return storageProvider;
			}
		}
		return nullptr;
	}
	const StorageProvider* SoundHoleMediaProvider::getStorageProvider(const String& name) const {
		for(auto storageProvider : storageProviders) {
			if(storageProvider->name() == name) {
				return storageProvider;
			}
		}
		return nullptr;
	}



	#pragma mark Login

	void SoundHoleMediaProvider::logout() {
		auto storageProvider = primaryStorageProvider();
		if(storageProvider == nullptr) {
			return;
		}
		storageProvider->logout();
	}

	bool SoundHoleMediaProvider::isLoggedIn() const {
		auto storageProvider = primaryStorageProvider();
		if(storageProvider == nullptr) {
			return false;
		}
		return storageProvider->isLoggedIn();
	}

	void SoundHoleMediaProvider::setPrimaryStorageProvider(StorageProvider* storageProvider) {
		if(storageProvider != nullptr) {
			primaryStorageProviderName = storageProvider->name();
		} else {
			primaryStorageProviderName = String();
		}
		saveUserPrefs();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> SoundHoleMediaProvider::getCurrentUserIds() {
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



	#pragma mark Data transforming

	Playlist::Data SoundHoleMediaProvider::createPlaylistData(StorageProvider* provider, StorageProvider::Playlist playlist) {
		return Playlist::Data{{{
			.partial = true,
			.type = "playlist",
			.name = playlist.name,
			.uri = createURI(provider->name(), "playlist", playlist.id),
			.images = std::nullopt // TODO get playlist images
			},
			.versionId = playlist.versionId,
			.itemCount = std::nullopt,
			.items = {}
			},
			.owner = playlist.owner ? this->userAccount(createUserData(provider, playlist.owner.value())) : nullptr,
			.privacy = Playlist::Privacy_fromString(playlist.privacy)
		};
	}

	UserAccount::Data SoundHoleMediaProvider::createUserData(StorageProvider* provider, StorageProvider::User user) {
		return UserAccount::Data{{
			.partial=true,
			.type="user",
			.name=user.name,
			.uri=createURI(provider->name(), "user", user.id),
			.images=ArrayList<MediaItem::Image>{
				MediaItem::Image{
					.url=user.imageURL,
					.size=MediaItem::Image::Size::MEDIUM
				}
			}
			},
			.id=user.id,
			.displayName=user.name
		};
	}



	#pragma mark Media Item Fetching

	Promise<Track::Data> SoundHoleMediaProvider::getTrackData(String uri) {
		return Promise<Track::Data>::reject(std::logic_error("SoundHoleMediaProvider::getTrackData is unimplemented"));
	}

	Promise<Artist::Data> SoundHoleMediaProvider::getArtistData(String uri) {
		return Promise<Artist::Data>::reject(std::logic_error("SoundHole provider does not have artists"));
	}

	Promise<Album::Data> SoundHoleMediaProvider::getAlbumData(String uri) {
		return Promise<Album::Data>::reject(std::logic_error("SoundHole provider does not have albums"));
	}

	Promise<Playlist::Data> SoundHoleMediaProvider::getPlaylistData(String uri) {
		auto uriParts = parseURI(uri);
		if(uriParts.type != "playlist") {
			return Promise<Playlist::Data>::reject(std::runtime_error("SoundHole URI "+uri+" is not a playlist URI"));
		}
		auto storageProvider = getStorageProvider(uriParts.storageProvider);
		if(storageProvider == nullptr) {
			return Promise<Playlist::Data>::reject(std::runtime_error("Could not find storage provider "+uriParts.storageProvider));
		}
		return storageProvider->getPlaylist(uriParts.id).map<Playlist::Data>([=](auto playlist) {
			return createPlaylistData(storageProvider,playlist);
		});
	}

	Promise<UserAccount::Data> SoundHoleMediaProvider::getUserData(String uri) {
		return Promise<UserAccount::Data>::reject(std::logic_error("SoundHoleMediaProvider::getUserData is unimplemented"));
	}



	Promise<ArrayList<$<Track>>> SoundHoleMediaProvider::getArtistTopTracks(String artistURI) {
		return Promise<ArrayList<$<Track>>>::reject(std::logic_error("SoundHole doesn't have artist top tracks"));
	}

	SoundHoleMediaProvider::ArtistAlbumsGenerator SoundHoleMediaProvider::getArtistAlbums(String artistURI) {
		using YieldResult = typename ArtistAlbumsGenerator::YieldResult;
		return ArtistAlbumsGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("SoundHole does not support artist albums"));
		});
	}

	SoundHoleMediaProvider::UserPlaylistsGenerator SoundHoleMediaProvider::getUserPlaylists(String userURI) {
		using YieldResult = typename UserPlaylistsGenerator::YieldResult;
		return UserPlaylistsGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("SoundHoleMediaProvider::getUserPlaylists is unimplemented"));
		});
	}



	Album::MutatorDelegate* SoundHoleMediaProvider::createAlbumMutatorDelegate($<Album> album) {
		throw std::logic_error("SoundHole does not support albums");
	}

	Playlist::MutatorDelegate* SoundHoleMediaProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		throw std::logic_error("SoundHoleMediaProvider::createPlaylistMutatorDelegate is unimplemented");
	}



	#pragma mark User Library

	bool SoundHoleMediaProvider::hasLibrary() const {
		return false;
	}

	SoundHoleMediaProvider::LibraryItemGenerator SoundHoleMediaProvider::generateLibrary(GenerateLibraryOptions options) {
		using YieldResult = typename LibraryItemGenerator::YieldResult;
		return LibraryItemGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("SoundHoleMediaProvider::generateLibrary is unimplemented"));
		});
	}



	#pragma mark Playlists

	bool SoundHoleMediaProvider::canCreatePlaylists() const {
		return true;
	}

	ArrayList<Playlist::Privacy> SoundHoleMediaProvider::supportedPlaylistPrivacies() const {
		return { Playlist::Privacy::PRIVATE, Playlist::Privacy::PUBLIC };
	}

	Promise<$<Playlist>> SoundHoleMediaProvider::createPlaylist(String name, CreatePlaylistOptions options) {
		return Promise<$<Playlist>>::reject(std::logic_error("SoundHoleMediaProvider::createPlaylist is unimplemented"));
	}

	Promise<bool> SoundHoleMediaProvider::isPlaylistEditable($<Playlist> playlist) {
		return Promise<bool>::reject(std::logic_error("SoundHoleMediaProvider::isPlaylistEditable is unimplemented"));
	}



	#pragma mark Player

	MediaPlaybackProvider* SoundHoleMediaProvider::player() {
		return nullptr;
	}

	const MediaPlaybackProvider* SoundHoleMediaProvider::player() const {
		return nullptr;
	}
}