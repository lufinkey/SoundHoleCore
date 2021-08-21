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
			String uniqueId;
			Date addedAt;
			$<UserAccount> addedBy;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<PlaylistItem> new$($<SpecialTrackCollection<PlaylistItem>> playlist, const Data& data);
		PlaylistItem($<SpecialTrackCollection<PlaylistItem>> playlist, const Data& data);
		
		const String& uniqueId() const;
		const Date& addedAt() const;
		$<UserAccount> addedBy();
		$<const UserAccount> addedBy() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		Data toData() const;
		virtual Json toJson() const override;
		
	protected:
		String _uniqueId;
		Date _addedAt;
		$<UserAccount> _addedBy;
	};


	class Playlist: public SpecialTrackCollection<PlaylistItem> {
	public:
		enum class Privacy: uint8_t {
			PRIVATE,
			UNLISTED,
			PUBLIC
		};
		static String Privacy_toString(Privacy);
		static Optional<Privacy> Privacy_fromString(const String&);
		
		struct Data: public SpecialTrackCollection<PlaylistItem>::Data {
			$<UserAccount> owner;
			Optional<Privacy> privacy;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<Playlist> new$(MediaProvider* provider, const Data& data);
		Playlist(MediaProvider* provider, const Data& data);
		
		$<UserAccount> owner();
		$<const UserAccount> owner() const;
		
		const Optional<Privacy>& privacy() const;
		
		Optional<bool> editable() const;
		Promise<bool> isEditable();
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData(const DataOptions& options = DataOptions()) const;
		virtual Json toJson(const ToJsonOptions& options) const override;
		
		Promise<void> insertItems(size_t index, LinkedList<$<Track>> items);
		Promise<void> appendItems(LinkedList<$<Track>> items);
		Promise<void> removeItems(size_t index, size_t count);
		Promise<void> moveItems(size_t index, size_t count, size_t newIndex);
		
	protected:
		virtual MutatorDelegate* createMutatorDelegate() override;
		
		$<UserAccount> _owner;
		Optional<Privacy> _privacy;
		Optional<bool> _editable;
	};
}
