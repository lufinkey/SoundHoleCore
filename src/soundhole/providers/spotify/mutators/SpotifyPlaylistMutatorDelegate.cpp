//
//  SpotifyPlaylistMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/15/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SpotifyPlaylistMutatorDelegate.hpp"
#include <soundhole/providers/spotify/SpotifyProvider.hpp>

namespace sh {
	SpotifyPlaylistMutatorDelegate::SpotifyPlaylistMutatorDelegate(SpotifyProvider* provider)
	: provider(provider) {
		//
	}

	Promise<void> SpotifyPlaylistMutatorDelegate::loadTrackCollectionItems($<TrackCollection> collection, Mutator* mutator, size_t index, size_t count) {
		auto playlist = std::static_pointer_cast<Playlist>(collection);
		auto id = provider->idFromURI(playlist->uri());
		return provider->spotify->getPlaylistTracks(id, {
			.offset=index,
			.limit=count
		}).then([=](SpotifyPage<SpotifyPlaylist::Item> page) -> void {
			auto items = page.items.map<$<PlaylistItem>>([&](auto& item) {
				return PlaylistItem::new$(playlist, {{
					.track=Track::new$(provider, provider->createTrackData(item.track))
					},
					.addedAt=provider->stringToTime(item.addedAt),
					.addedBy=UserAccount::new$(provider, provider->createUserAccountData(item.addedBy))
				});
			});
			mutator->lock([&]() {
				mutator->resize(page.total);
				mutator->apply(page.offset, items);
			});
		});
	}
}
