//
//  GoogleDrivePlaylistMutatorDelegate.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/23/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/Playlist.hpp>
#include <soundhole/storage/googledrive/api/GoogleDriveStorageMediaTypes.hpp>

namespace sh {
	class GoogleDriveStorageProvider;

	class GoogleDrivePlaylistMutatorDelegate: public Playlist::MutatorDelegate {
		friend class GoogleDriveStorageProvider;
	public:
		virtual size_t getChunkSize() const override;
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) override;
		virtual Promise<void> insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) override;
		virtual Promise<void> appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) override;
		virtual Promise<void> removeItems(Mutator* mutator, size_t index, size_t count) override;
		virtual Promise<void> moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) override;
		
	protected:
		Promise<void> loadAPIItems(Mutator* mutator, size_t index, size_t count);
		
	private:
		GoogleDrivePlaylistMutatorDelegate($<Playlist> playlist, GoogleDriveStorageProvider* storageProvider);
		
		w$<Playlist> playlist;
		GoogleDriveStorageProvider* storageProvider;
	};
}
