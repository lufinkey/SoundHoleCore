//
//  soundhole.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 7/8/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include <soundhole/soundhole.hpp>

#ifndef FGL_ASYNCLIST_USED_DTL
#error "SoundHoleCore requires use of DTL in AsyncList"
#endif

namespace sh {
	SoundHole::SoundHole(Options options)
	: _mediaLibrary(nullptr), _streamPlayer(nullptr), _player(nullptr) {
		_streamPlayer = StreamPlayer::new$();
		
		if(options.soundhole) {
			_mediaProviders.pushBack(new SoundHoleMediaProvider(this, options.soundhole.value()));
		}
		if(options.spotify) {
			_mediaProviders.pushBack(new SpotifyMediaProvider(options.spotify.value()));
		}
		if(options.bandcamp) {
			_mediaProviders.pushBack(new BandcampMediaProvider(options.bandcamp.value()));
		}
		if(options.youtube) {
			if(options.youtube->libraryPlaylistName.empty()) {
				options.youtube->libraryPlaylistName = "SoundHole Library";
			}
			_mediaProviders.pushBack(new YoutubeMediaProvider(options.youtube.value()));
		}
		if(options.lastfm) {
			auto lastfm = new LastFMMediaProvider(options.lastfm.value());
			_mediaProviders.pushBack(lastfm);
			_scrobblers.pushBack(lastfm);
		}
		
		_mediaLibrary = new MediaLibrary({
			.dbPath = options.dbPath,
			.mediaProviderStash = this
		});
		
		if(options.player) {
			_player = Player::new$(_mediaLibrary->database(), _streamPlayer, options.player.value());
		}
	}

	SoundHole::~SoundHole() {
		for(auto provider : _mediaProviders) {
			auto scrobbler = dynamic_cast<Scrobbler*>(provider);
			if(scrobbler != nullptr) {
				_scrobblers.removeFirstEqual(scrobbler);
			}
			delete provider;
		}
		for(auto scrobbler : _scrobblers) {
			delete scrobbler;
		}
		delete _mediaLibrary;
	}



	Promise<void> SoundHole::initialize() {
		return _mediaLibrary->initialize().then([=]() {
			if(_player != nullptr) {
				_player->load(this);
			}
		});
	}



	MediaLibrary* SoundHole::mediaLibrary() {
		return _mediaLibrary;
	}

	const MediaLibrary* SoundHole::mediaLibrary() const {
		return _mediaLibrary;
	}



	$<StreamPlayer> SoundHole::streamPlayer() {
		return _streamPlayer;
	}

	$<const StreamPlayer> SoundHole::streamPlayer() const {
		return _streamPlayer;
	}



	$<Player> SoundHole::player() {
		return _player;
	}

	$<const Player> SoundHole::player() const {
		return _player;
	}



	void SoundHole::addMediaProvider(MediaProvider* mediaProvider) {
		_mediaProviders.pushBack(mediaProvider);
	}

	$<MediaItem> SoundHole::parseMediaItem(const Json& json) {
		if(json.is_null()) {
			return nullptr;
		}
		auto providerName = json["provider"];
		if(!providerName.is_string()) {
			throw std::invalid_argument("provider must be a string");
		}
		auto provider = getMediaProvider(providerName.string_value());
		if(provider == nullptr) {
			throw std::invalid_argument("invalid provider name: "+providerName.string_value());
		}
		auto type = json["type"].string_value();
		if(type.empty()) {
			throw std::invalid_argument("No type stored for media item json");
		}
		if(type == "track") {
			return provider->track(Track::Data::fromJson(json,this));
		} else if(type == "artist" || type == "label") {
			return provider->artist(Artist::Data::fromJson(json,this));
		} else if(type == "album") {
			return provider->album(Album::Data::fromJson(json,this));
		} else if(type == "playlist") {
			return provider->playlist(Playlist::Data::fromJson(json,this));
		} else if(type == "user") {
			return provider->userAccount(UserAccount::Data::fromJson(json,this));
		} else if(type == MediaLibraryTracksCollection::TYPE && provider == _mediaLibrary->proxyProvider()) {
			auto libraryProvider = static_cast<MediaLibraryProxyProvider*>(provider);
			return libraryProvider->libraryTracksCollection(MediaLibraryTracksCollection::Data::fromJson(json, this));
		}
		throw std::invalid_argument("invalid media item type "+type);
	}

	MediaProvider* SoundHole::getMediaProvider(const String& name) {
		for(auto provider : _mediaProviders) {
			if(provider->name() == name) {
				return provider;
			}
		}
		if(_mediaLibrary->proxyProvider()->name() == name) {
			return _mediaLibrary->proxyProvider();
		}
		return nullptr;
	}

	ArrayList<MediaProvider*> SoundHole::getMediaProviders() {
		return _mediaProviders;
	}


	ArrayList<Scrobbler*> SoundHole::getScrobblers() {
		return _scrobblers;
	}


	bool SoundHole::isSynchronizingLibrary(const String& libraryProviderName) {
		return _mediaLibrary->isSynchronizingLibrary(libraryProviderName);
	}
	bool SoundHole::isSynchronizingLibraries() {
		return _mediaLibrary->isSynchronizingLibraries();
	}

	Optional<AsyncQueue::TaskNode> SoundHole::getSynchronizeLibraryTask(const String& libraryProviderName) {
		return _mediaLibrary->getSynchronizeLibraryTask(libraryProviderName);
	}
	Optional<AsyncQueue::TaskNode> SoundHole::getSynchronizeAllLibrariesTask() {
		return _mediaLibrary->getSynchronizeAllLibrariesTask();
	}

	AsyncQueue::TaskNode SoundHole::synchronizeProviderLibrary(const String& libraryProviderName) {
		return _mediaLibrary->synchronizeProviderLibrary(libraryProviderName);
	}
	AsyncQueue::TaskNode SoundHole::synchronizeProviderLibrary(MediaProvider* libraryProvider) {
		return _mediaLibrary->synchronizeProviderLibrary(libraryProvider);
	}

	AsyncQueue::TaskNode SoundHole::synchronizeAllLibraries() {
		return _mediaLibrary->synchronizeAllLibraries();
	}

	Promise<$<MediaLibraryTracksCollection>> SoundHole::getLibraryTracksCollection(GetLibraryTracksCollectionOptions options) {
		return _mediaLibrary->getLibraryTracksCollection(options);
	}

	Promise<SoundHole::GetPlaybackHistoryItemsResult> SoundHole::getPlaybackHistoryItems(GetPlaybackHistoryItemsOptions options) {
		return _mediaLibrary->getPlaybackHistoryItems(options);
	}

	SoundHole::PlaybackHistoryItemGenerator SoundHole::generatePlaybackHistoryItems(GeneratePlaybackHistoryItemsOptions options) {
		return _mediaLibrary->generatePlaybackHistoryItems(options);
	}

	Promise<$<PlaybackHistoryTrackCollection>> SoundHole::getPlaybackHistoryCollection(GetPlaybackHistoryCollectionOptions options) {
		return _mediaLibrary->getPlaybackHistoryCollection(options);
	}

	Promise<LinkedList<$<Album>>> SoundHole::getLibraryAlbums(LibraryAlbumsFilters filters) {
		return _mediaLibrary->getLibraryAlbums(filters);
	}

	SoundHole::LibraryAlbumsGenerator SoundHole::generateLibraryAlbums(GenerateLibraryAlbumsOptions options) {
		return _mediaLibrary->generateLibraryAlbums(options);
	}

	Promise<LinkedList<$<Playlist>>> SoundHole::getLibraryPlaylists(LibraryPlaylistsFilters filters) {
		return _mediaLibrary->getLibraryPlaylists(filters);
	}

	SoundHole::LibraryPlaylistsGenerator SoundHole::generateLibraryPlaylists(GenerateLibraryPlaylistsOptions options) {
		return _mediaLibrary->generateLibraryPlaylists(options);
	}



	Promise<$<Playlist>> SoundHole::createPlaylist(String name, MediaProvider* provider, CreatePlaylistOptions options) {
		return _mediaLibrary->createPlaylist(name, provider, options);
	}
}
