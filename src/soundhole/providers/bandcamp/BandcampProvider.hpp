//
//  BandcampProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <tuple>
#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include "BandcampPlaybackProvider.hpp"
#include "api/Bandcamp.hpp"

namespace sh {
	class BandcampProvider: public MediaProvider {
		friend class BandcampAlbumMutatorDelegate;
	public:
		using Options = Bandcamp::Options;
		
		BandcampProvider(Options options);
		virtual ~BandcampProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		Bandcamp* api();
		const Bandcamp* api() const;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserIds() override;
		
		struct SearchResults {
			String prevURL;
			String nextURL;
			LinkedList<$<MediaItem>> items;
		};
		using SearchOptions = Bandcamp::SearchOptions;
		Promise<SearchResults> search(String query, SearchOptions options={.page=0});
		
		virtual Promise<Track::Data> getTrackData(String uri) override;
		virtual Promise<Artist::Data> getArtistData(String uri) override;
		virtual Promise<Album::Data> getAlbumData(String uri) override;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) override;
		virtual Promise<UserAccount::Data> getUserData(String uri) override;
		
		virtual Promise<ArrayList<$<Track>>> getArtistTopTracks(String artistURI) override;
		Promise<std::tuple<$<Artist>,LinkedList<$<Album>>>> getArtistAndAlbums(String artistURI);
		virtual ArtistAlbumsGenerator getArtistAlbums(String artistURI) override;
		
		virtual UserPlaylistsGenerator getUserPlaylists(String userURI) override;
		
		virtual Album::MutatorDelegate* createAlbumMutatorDelegate($<Album> album) override;
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) override;
		
		virtual bool hasLibrary() const override;
		virtual LibraryItemGenerator generateLibrary(GenerateLibraryOptions options = GenerateLibraryOptions()) override;
		
		virtual BandcampPlaybackProvider* player() override;
		virtual const BandcampPlaybackProvider* player() const override;
		
		Track::Data createTrackData(BandcampTrack track, bool partial);
		Artist::Data createArtistData(BandcampArtist artist, bool partial);
		Album::Data createAlbumData(BandcampAlbum album, bool partial);
		UserAccount::Data createUserData(BandcampFan fan);
		
		struct URI {
			String provider;
			String url;
		};
		URI parseURI(String uri) const;
		
		Promise<Optional<BandcampIdentities>> getCurrentBandcampIdentities();
		
	private:
		URI parseURL(String url) const;
		String createURI(String type, String url) const;
		
		static MediaItem::Image createImage(BandcampImage image);
		
		String getCachedCurrentBandcampIdentitiesPath() const;
		void setCurrentBandcampIdentities(Optional<BandcampIdentities> identities);
		
		Bandcamp* bandcamp;
		BandcampPlaybackProvider* _player;
		
		Optional<BandcampIdentities> _currentIdentities;
		Optional<Promise<Optional<BandcampIdentities>>> _currentIdentitiesPromise;
		bool _currentIdentitiesNeedRefresh;
	};
}
