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
	SoundHole::SoundHole(Options options) {
		if(options.soundhole) {
			mediaProviders.pushBack(new SoundHoleMediaProvider(this, options.soundhole.value()));
		}
		if(options.spotify) {
			mediaProviders.pushBack(new SpotifyMediaProvider(options.spotify.value()));
		}
		if(options.bandcamp) {
			mediaProviders.pushBack(new BandcampMediaProvider(options.bandcamp.value()));
		}
		if(options.youtube) {
			mediaProviders.pushBack(new YoutubeMediaProvider(options.youtube.value()));
		}
	}

	SoundHole::~SoundHole() {
		for(auto provider : mediaProviders) {
			delete provider;
		}
	}

	void SoundHole::addMediaProvider(MediaProvider* mediaProvider) {
		mediaProviders.pushBack(mediaProvider);
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
		} else if(type == "artist") {
			return provider->artist(Artist::Data::fromJson(json,this));
		} else if(type == "album" || type == "label") {
			return provider->album(Album::Data::fromJson(json,this));
		} else if(type == "playlist") {
			return provider->playlist(Playlist::Data::fromJson(json,this));
		} else if(type == "user") {
			return provider->userAccount(UserAccount::Data::fromJson(json,this));
		} else if(type == MediaLibraryTracksCollection::TYPE && provider == mediaLibrary->proxyProvider()) {
			auto libraryProvider = static_cast<MediaLibraryProxyProvider*>(provider);
			return libraryProvider->libraryTracksCollection(MediaLibraryTracksCollection::Data::fromJson(json, this));
		}
		throw std::invalid_argument("invalid media item type "+type);
	}

	MediaProvider* SoundHole::getMediaProvider(const String& name) {
		for(auto provider : mediaProviders) {
			if(provider->name() == name) {
				return provider;
			}
		}
		if(mediaLibrary->proxyProvider()->name() == name) {
			return mediaLibrary->proxyProvider();
		}
		return nullptr;
	}

	ArrayList<MediaProvider*> SoundHole::getMediaProviders() {
		return mediaProviders;
	}

	bool SoundHole::isSynchronizingLibrary(const String& libraryProviderName) {
		return mediaLibrary->isSynchronizingLibrary(libraryProviderName);
	}
	bool SoundHole::isSynchronizingLibraries() {
		return mediaLibrary->isSynchronizingLibraries();
	}

	Optional<AsyncQueue::TaskNode> SoundHole::getSynchronizeLibraryTask(const String& libraryProviderName) {
		return mediaLibrary->getSynchronizeLibraryTask(libraryProviderName);
	}
	Optional<AsyncQueue::TaskNode> SoundHole::getSynchronizeAllLibrariesTask() {
		return mediaLibrary->getSynchronizeAllLibrariesTask();
	}

	AsyncQueue::TaskNode SoundHole::synchronizeProviderLibrary(const String& libraryProviderName) {
		return mediaLibrary->synchronizeProviderLibrary(libraryProviderName);
	}
	AsyncQueue::TaskNode SoundHole::synchronizeProviderLibrary(MediaProvider* libraryProvider) {
		return mediaLibrary->synchronizeProviderLibrary(libraryProvider);
	}

	AsyncQueue::TaskNode SoundHole::synchronizeAllLibraries() {
		return mediaLibrary->synchronizeAllLibraries();
	}

	Promise<$<MediaLibraryTracksCollection>> SoundHole::getLibraryTracksCollection(GetLibraryTracksOptions options) {
		return mediaLibrary->getLibraryTracksCollection(options);
	}

	Promise<LinkedList<$<Album>>> SoundHole::getLibraryAlbums(LibraryAlbumsFilters filters) {
		return mediaLibrary->getLibraryAlbums(filters);
	}

	SoundHole::LibraryAlbumsGenerator SoundHole::generateLibraryAlbums(GenerateLibraryAlbumsOptions options) {
		return mediaLibrary->generateLibraryAlbums(options);
	}

	Promise<LinkedList<$<Playlist>>> SoundHole::getLibraryPlaylists(LibraryPlaylistsFilters filters) {
		return mediaLibrary->getLibraryPlaylists(filters);
	}

	SoundHole::LibraryPlaylistsGenerator SoundHole::generateLibraryPlaylists(GenerateLibraryPlaylistsOptions options) {
		return mediaLibrary->generateLibraryPlaylists(options);
	}
}
