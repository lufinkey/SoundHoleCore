//
//  SoundHoleMediaProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/27/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SoundHoleMediaProvider.hpp"

namespace sh {
	SoundHoleMediaProvider::SoundHoleMediaProvider(MediaProviderStash* mediaProviderStash, Options options)
	: mediaProviderStash(mediaProviderStash), sessionPersistKey(options.auth.sessionPersistKey) {
		if(options.googledrive) {
			auto googledrive = new GoogleDriveStorageProvider(this, mediaProviderStash, options.googledrive.value());
			storageProviders.pushBack(googledrive);
		}
		loadUserPrefs();
		if(primaryStorageProviderName.empty() && storageProviders.size() > 0) {
			primaryStorageProviderName = storageProviders.front()->name();
		}
	}

	SoundHoleMediaProvider::~SoundHoleMediaProvider() {
		for(auto storageProvider : storageProviders) {
			delete storageProvider;
		}
	}



	String SoundHoleMediaProvider::name() const {
		return NAME;
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

	ArrayList<StorageProvider*> SoundHoleMediaProvider::getStorageProviders() {
		return storageProviders;
	}

	ArrayList<const StorageProvider*> SoundHoleMediaProvider::getStorageProviders() const {
		return reinterpret_cast<const ArrayList<const StorageProvider*>&>(storageProviders);
	}

	void SoundHoleMediaProvider::addStorageProvider(StorageProvider* storageProvider) {
		storageProviders.pushBack(storageProvider);
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

	Promise<ArrayList<String>> SoundHoleMediaProvider::getCurrentUserURIs() {
		auto promises = ArrayList<Promise<ArrayList<String>>>();
		promises.reserve(storageProviders.size());
		for(auto storageProvider : storageProviders) {
			auto providerName = storageProvider->name();
			promises.pushBack(storageProvider->getCurrentUserURIs());
		}
		return Promise<ArrayList<String>>::all(promises).map([=](auto userURILists) -> ArrayList<String> {
			auto userURIs = ArrayList<String>();
			size_t count = 0;
			for(auto& list : userURILists) {
				count += list.size();
			}
			userURIs.reserve(count);
			for(auto& list : userURILists) {
				userURIs.pushBackList(std::move(list));
			}
			return userURIs;
		});
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
		return storageProvider->getPlaylistData(uri);
	}

	Promise<UserAccount::Data> SoundHoleMediaProvider::getUserData(String uri) {
		auto uriParts = parseURI(uri);
		if(uriParts.type != "user") {
			return Promise<UserAccount::Data>::reject(std::runtime_error("SoundHole URI "+uri+" is not a user URI"));
		}
		auto storageProvider = getStorageProvider(uriParts.storageProvider);
		if(storageProvider == nullptr) {
			return Promise<UserAccount::Data>::reject(std::runtime_error("Could not find storage provider "+uriParts.storageProvider));
		}
		return storageProvider->getUserData(uri);
	}



	$<Track> SoundHoleMediaProvider::track(const Track::Data& data) {
		return MediaProvider::track(data);
	}
	$<Artist> SoundHoleMediaProvider::artist(const Artist::Data& data) {
		return MediaProvider::artist(data);
	}
	$<Album> SoundHoleMediaProvider::album(const Album::Data& data) {
		return MediaProvider::album(data);
	}
	$<Playlist> SoundHoleMediaProvider::playlist(const Playlist::Data& data) {
		return MediaProvider::playlist(data);
	}
	$<UserAccount> SoundHoleMediaProvider::userAccount(const UserAccount::Data& data) {
		return MediaProvider::userAccount(data);
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
		auto uriParts = parseURI(userURI);
		auto storageProvider = getStorageProvider(uriParts.storageProvider);
		return storageProvider->getUserPlaylists(userURI);
	}



	Album::MutatorDelegate* SoundHoleMediaProvider::createAlbumMutatorDelegate($<Album> album) {
		throw std::logic_error("SoundHoleMediaProvider does not support albums");
	}

	Playlist::MutatorDelegate* SoundHoleMediaProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		auto uriParts = parseURI(playlist->uri());
		auto storageProvider = this->getStorageProvider(uriParts.storageProvider);
		return storageProvider->createPlaylistMutatorDelegate(playlist);
	}



	#pragma mark User Library

	bool SoundHoleMediaProvider::hasLibrary() const {
		return (this->primaryStorageProvider() != nullptr);
	}

	SoundHoleMediaProvider::LibraryItemGenerator SoundHoleMediaProvider::generateLibrary(GenerateLibraryOptions options) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			using YieldResult = typename LibraryItemGenerator::YieldResult;
			return LibraryItemGenerator([=]() {
				return Promise<YieldResult>::resolve({
					.value = std::nullopt,
					.done = true
				});
			});
		}
		return primaryStorageProvider->generateLibrary(options);
	}



	bool SoundHoleMediaProvider::canFollowArtists() const {
		return true;
	}

	Promise<void> SoundHoleMediaProvider::followArtist(String artistURI) {
		auto uriParts = parseURI(artistURI);
		if(uriParts.provider != this->name()) {
			return Promise<void>::reject(std::invalid_argument("invalid artistURI "+artistURI+" is not for provider "+this->name()));
		}
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->followArtists({
			StorageProvider::NewFollowedItem{
				.uri = artistURI,
				.provider = this->name()
			}
		});
	}

	Promise<void> SoundHoleMediaProvider::unfollowArtist(String artistURI) {
		auto uriParts = parseURI(artistURI);
		if(uriParts.provider != this->name()) {
			return Promise<void>::reject(std::invalid_argument("invalid artistURI "+artistURI+" is not for provider "+this->name()));
		}
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->unfollowArtists({ artistURI });
	}

	Promise<void> SoundHoleMediaProvider::followArtist(String artistURI, MediaProvider* provider) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->followArtists({
			StorageProvider::NewFollowedItem{
				.uri = artistURI,
				.provider = provider->name()
			}
		});
	}

	Promise<void> SoundHoleMediaProvider::unfollowArtist(String artistURI, MediaProvider* provider) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->unfollowArtists({ artistURI });
	}



	bool SoundHoleMediaProvider::canFollowUsers() const {
		return true;
	}

	Promise<void> SoundHoleMediaProvider::followUser(String userURI) {
		auto uriParts = parseURI(userURI);
		if(uriParts.provider != this->name()) {
			return Promise<void>::reject(std::invalid_argument("invalid userURI "+userURI+" is not for provider "+this->name()));
		}
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->followUsers({
			StorageProvider::NewFollowedItem{
				.uri = userURI,
				.provider = this->name()
			}
		});
	}

	Promise<void> SoundHoleMediaProvider::unfollowUser(String userURI) {
		auto uriParts = parseURI(userURI);
		if(uriParts.provider != this->name()) {
			return Promise<void>::reject(std::invalid_argument("invalid userURI "+userURI+" is not for provider "+this->name()));
		}
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->unfollowUsers({ userURI });
	}

	Promise<void> SoundHoleMediaProvider::followUser(String userURI, MediaProvider* provider) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->followUsers({
			StorageProvider::NewFollowedItem{
				.uri = userURI,
				.provider = provider->name()
			}
		});
	}

	Promise<void> SoundHoleMediaProvider::unfollowUser(String userURI, MediaProvider* provider) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->unfollowUsers({ userURI });
	}



	bool SoundHoleMediaProvider::canSaveTracks() const {
		return true;
	}

	Promise<void> SoundHoleMediaProvider::saveTrack(String trackURI) {
		auto uriParts = parseURI(trackURI);
		if(uriParts.provider != this->name()) {
			return Promise<void>::reject(std::invalid_argument("invalid trackURI "+trackURI+" is not for provider "+this->name()));
		}
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->saveTracks({
			StorageProvider::NewFollowedItem{
				.uri = trackURI,
				.provider = this->name()
			}
		});
	}

	Promise<void> SoundHoleMediaProvider::unsaveTrack(String trackURI) {
		auto uriParts = parseURI(trackURI);
		if(uriParts.provider != this->name()) {
			return Promise<void>::reject(std::invalid_argument("invalid trackURI "+trackURI+" is not for provider "+this->name()));
		}
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->unsaveTracks({ trackURI });
	}

	Promise<void> SoundHoleMediaProvider::saveTrack(String trackURI, MediaProvider* provider) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->saveTracks({
			StorageProvider::NewFollowedItem{
				.uri = trackURI,
				.provider = provider->name()
			}
		});
	}

	Promise<void> SoundHoleMediaProvider::unsaveTrack(String trackURI, MediaProvider* provider) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->unsaveTracks({ trackURI });
	}



	bool SoundHoleMediaProvider::canSaveAlbums() const {
		return true;
	}

	Promise<void> SoundHoleMediaProvider::saveAlbum(String albumURI) {
		auto uriParts = parseURI(albumURI);
		if(uriParts.provider != this->name()) {
			return Promise<void>::reject(std::invalid_argument("invalid albumURI "+albumURI+" is not for provider "+this->name()));
		}
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->saveAlbums({
			StorageProvider::NewFollowedItem{
				.uri = albumURI,
				.provider = this->name()
			}
		});
	}

	Promise<void> SoundHoleMediaProvider::unsaveAlbum(String albumURI) {
		auto uriParts = parseURI(albumURI);
		if(uriParts.provider != this->name()) {
			return Promise<void>::reject(std::invalid_argument("invalid albumURI "+albumURI+" is not for provider "+this->name()));
		}
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->unsaveAlbums({ albumURI });
	}

	Promise<void> SoundHoleMediaProvider::saveAlbum(String albumURI, MediaProvider* provider) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->saveAlbums({
			StorageProvider::NewFollowedItem{
				.uri = albumURI,
				.provider = provider->name()
			}
		});
	}

	Promise<void> SoundHoleMediaProvider::unsaveAlbum(String albumURI, MediaProvider* provider) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->unsaveAlbums({ albumURI });
	}



	bool SoundHoleMediaProvider::canSavePlaylists() const {
		return true;
	}

	Promise<void> SoundHoleMediaProvider::savePlaylist(String playlistURI) {
		auto uriParts = parseURI(playlistURI);
		if(uriParts.provider != this->name()) {
			return Promise<void>::reject(std::invalid_argument("invalid playlistURI "+playlistURI+" is not for provider "+this->name()));
		}
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->followPlaylists({
			StorageProvider::NewFollowedItem{
				.uri = playlistURI,
				.provider = this->name()
			}
		});
	}

	Promise<void> SoundHoleMediaProvider::unsavePlaylist(String playlistURI) {
		auto uriParts = parseURI(playlistURI);
		if(uriParts.provider != this->name()) {
			return Promise<void>::reject(std::invalid_argument("invalid playlistURI "+playlistURI+" is not for provider "+this->name()));
		}
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->unfollowPlaylists({ playlistURI });
	}

	Promise<void> SoundHoleMediaProvider::savePlaylist(String playlistURI, MediaProvider* provider) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->followPlaylists({
			StorageProvider::NewFollowedItem{
				.uri = playlistURI,
				.provider = provider->name()
			}
		});
	}

	Promise<void> SoundHoleMediaProvider::unsavePlaylist(String playlistURI, MediaProvider* provider) {
		auto primaryStorageProvider = this->primaryStorageProvider();
		if(primaryStorageProvider == nullptr) {
			return Promise<void>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return primaryStorageProvider->unfollowPlaylists({ playlistURI });
	}



	#pragma mark Playlists

	bool SoundHoleMediaProvider::canCreatePlaylists() const {
		auto storageProvider = primaryStorageProvider();
		if(storageProvider == nullptr) {
			return false;
		}
		return true;
	}

	ArrayList<Playlist::Privacy> SoundHoleMediaProvider::supportedPlaylistPrivacies() const {
		return { Playlist::Privacy::PRIVATE, Playlist::Privacy::UNLISTED, Playlist::Privacy::PUBLIC };
	}

	Promise<$<Playlist>> SoundHoleMediaProvider::createPlaylist(String name, CreatePlaylistOptions options) {
		auto storageProvider = primaryStorageProvider();
		if(storageProvider == nullptr) {
			return Promise<$<Playlist>>::reject(std::runtime_error("No primary storage provider has been set"));
		}
		return storageProvider->createPlaylist(name, options);
	}

	Promise<bool> SoundHoleMediaProvider::isPlaylistEditable($<Playlist> playlist) {
		auto uriParts = parseURI(playlist->uri());
		auto storageProvider = getStorageProvider(uriParts.storageProvider);
		if(storageProvider == nullptr) {
			return Promise<bool>::reject(std::invalid_argument("could not find storage provider with name "+uriParts.storageProvider));
		}
		return storageProvider->isPlaylistEditable(playlist);
	}

	Promise<void> SoundHoleMediaProvider::deletePlaylist(String playlistURI) {
		auto uriParts = parseURI(playlistURI);
		auto storageProvider = getStorageProvider(uriParts.storageProvider);
		if(storageProvider == nullptr) {
			return Promise<void>::reject(std::invalid_argument("could not find storage provider with name "+uriParts.storageProvider));
		}
		return storageProvider->deletePlaylist(playlistURI);
	}



	#pragma mark Player

	MediaPlaybackProvider* SoundHoleMediaProvider::player() {
		return nullptr;
	}

	const MediaPlaybackProvider* SoundHoleMediaProvider::player() const {
		return nullptr;
	}



	#pragma mark StorageProvider::URI

	StorageProvider::URI SoundHoleMediaProvider::parseStorageProviderURI(String uri) const {
		auto uriParts = parseURI(uri);
		return StorageProvider::URI{
			.storageProvider = uriParts.storageProvider,
			.type = uriParts.type,
			.id = uriParts.id
		};
	}

	String SoundHoleMediaProvider::createStorageProviderURI(String storageProvider, String type, String id) const {
		return createURI(storageProvider, type, id);
	}
}
