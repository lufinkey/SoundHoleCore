//
//  Playlist.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "TrackCollection.hpp"
#include "UserAccount.hpp"

namespace sh {
	class Playlist;


	class PlaylistItem: public SpecialTrackCollectionItem<Playlist> {
	public:
		struct Data: public SpecialTrackCollectionItem<Playlist>::Data {
			String addedAt;
			$<UserAccount> addedBy;
		};
		
		static $<PlaylistItem> new$($<Playlist> playlist, Data data);
		static $<PlaylistItem> new$($<SpecialTrackCollection<PlaylistItem>> playlist, Data data);
		
		PlaylistItem($<Playlist> playlist, Data data);
		
		const String& addedAt() const;
		$<UserAccount> addedBy();
		$<const UserAccount> addedBy() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		Data toData() const;
		
	protected:
		String _addedAt;
		$<UserAccount> _addedBy;
	};


	class Playlist: public SpecialTrackCollection<PlaylistItem> {
	public:
		struct Data: public SpecialTrackCollection<PlaylistItem>::Data {
			$<UserAccount> owner;
		};
		
		static $<Playlist> new$(MediaProvider* provider, Data data);
		
		Playlist(MediaProvider* provider, Data data);
		
		$<UserAccount> owner();
		$<const UserAccount> owner() const;
		
		virtual bool needsData() const override;
		virtual Promise<void> fetchMissingData() override;
		
		Data toData(DataOptions options = DataOptions()) const;
		
	protected:
		virtual MutatorDelegate* createMutatorDelegate() override;
		
	private:
		$<UserAccount> _owner;
	};
}
