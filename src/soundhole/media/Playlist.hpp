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
		
		static $<PlaylistItem> new$($<SpecialTrackCollection<PlaylistItem>> playlist, Data data);
		
		PlaylistItem($<SpecialTrackCollection<PlaylistItem>> playlist, Data data);
		PlaylistItem($<SpecialTrackCollection<PlaylistItem>> playlist, Json json, MediaProviderStash* stash);
		
		const String& addedAt() const;
		$<UserAccount> addedBy();
		$<const UserAccount> addedBy() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		Data toData() const;
		
		static $<PlaylistItem> fromJson($<SpecialTrackCollection<PlaylistItem>> playlist, Json json, MediaProviderStash* stash);
		virtual Json toJson() const override;
		
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
		Playlist(Json json, MediaProviderStash* stash);
		
		$<UserAccount> owner();
		$<const UserAccount> owner() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData(DataOptions options = DataOptions()) const;
		
		static $<Playlist> fromJson(Json json, MediaProviderStash* stash);
		virtual Json toJson(ToJsonOptions options) const override;
		
	protected:
		virtual MutatorDelegate* createMutatorDelegate() override;
		
		$<UserAccount> _owner;
	};
}
