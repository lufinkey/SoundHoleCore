//
//  YoutubePlaylistMutatorDelegate.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/17/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/Playlist.hpp>

namespace sh {
	class YoutubeMediaProvider;

	class YoutubePlaylistMutatorDelegate: public Playlist::MutatorDelegate {
		friend class YoutubeMediaProvider;
	public:
		virtual size_t getChunkSize() const override;
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) override;
		
		virtual bool canInsertItem($<Track> item) const override;
		virtual Promise<void> insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks, InsertItemOptions options) override;
		virtual Promise<void> appendItems(Mutator* mutator, LinkedList<$<Track>> tracks, InsertItemOptions options) override;
		virtual Promise<void> removeItems(Mutator* mutator, size_t index, size_t count) override;
		virtual Promise<void> moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) override;
		
	private:
		YoutubePlaylistMutatorDelegate($<Playlist> playlist);
		
		struct Range {
			size_t index;
			size_t count;
		};
		ArrayList<Range> coverRange(Range range, Range coverRange) const;
		struct Dist {
			size_t count;
			bool reverse;
		};
		ArrayList<Dist> coverRangeDist(Range range, Range coverRange) const;
		
		struct LoadPager {
			String prevPageToken;
			String nextPageToken;
		};
		Promise<LoadPager> loadItems(Mutator* mutator, String pageToken);
		Promise<void> loadItemsToIndex(Mutator* mutator, size_t targetIndex, String pageToken, size_t currentIndex, bool reverse);
		
		w$<Playlist> playlist;
		
		size_t pageSize;
		struct PageToken {
			String value;
			size_t offset;
		};
		LinkedList<PageToken> pageTokens;
	};
}
