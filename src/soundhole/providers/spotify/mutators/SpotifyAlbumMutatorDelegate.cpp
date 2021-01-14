//
//  SpotifyAlbumMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/15/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SpotifyAlbumMutatorDelegate.hpp"
#include <soundhole/providers/spotify/SpotifyMediaProvider.hpp>
#include <soundhole/database/MediaDatabase.hpp>

namespace sh {
	SpotifyAlbumMutatorDelegate::SpotifyAlbumMutatorDelegate($<Album> album)
	: album(album) {
		//
	}

	size_t SpotifyAlbumMutatorDelegate::getChunkSize() const {
		return 50;
	}

	Promise<void> SpotifyAlbumMutatorDelegate::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		auto album = this->album.lock();
		if(options.offline && options.database != nullptr) {
			// offline load
			return options.database->loadAlbumItems(album, mutator, index, count);
		}
		else {
			// online load
			auto provider = (SpotifyMediaProvider*)album->mediaProvider();
			auto uriParts = provider->parseURI(album->uri());
			return provider->spotify->getAlbumTracks(uriParts.id, {
				.market="from_token",
				.offset=index,
				.limit=count
			}).then([=](SpotifyPage<SpotifyTrack> page) -> void {
				auto items = page.items.map<$<AlbumItem>>([&](auto& track) {
					auto trackData = provider->createTrackData(track, true);
					trackData.albumName = album->name();
					trackData.albumURI = album->uri();
					if(!trackData.images) {
						trackData.images = album->images();
					}
					return album->createCollectionItem({
						.track=provider->track(trackData)
					});
				});
				mutator->lock([&]() {
					mutator->applyAndResize(page.offset, page.total, items);
				});
			});
		}
	}


	Promise<void> SpotifyAlbumMutatorDelegate::insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) {
		return Promise<void>::reject(std::runtime_error("Cannot insert items into an album"));
	}

	Promise<void> SpotifyAlbumMutatorDelegate::appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) {
		return Promise<void>::reject(std::runtime_error("Cannot append items to an album"));
	}

	Promise<void> SpotifyAlbumMutatorDelegate::removeItems(Mutator* mutator, size_t index, size_t count) {
		return Promise<void>::reject(std::runtime_error("Cannot remove items from an album"));
	}

	Promise<void> SpotifyAlbumMutatorDelegate::moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) {
		return Promise<void>::reject(std::runtime_error("Cannot move items inside an album"));
	}
}
