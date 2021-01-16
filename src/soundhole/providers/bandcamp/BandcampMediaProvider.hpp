//
//  BandcampMediaProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <tuple>
#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include <soundhole/media/AuthedProviderIdentityStore.hpp>
#include "BandcampPlaybackProvider.hpp"
#include "api/Bandcamp.hpp"

namespace sh {
	class BandcampMediaProvider: public MediaProvider, public AuthedProviderIdentityStore<BandcampIdentities> {
		friend class BandcampAlbumMutatorDelegate;
	public:
		using Options = Bandcamp::Options;
		
		BandcampMediaProvider(Options options);
		virtual ~BandcampMediaProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		Bandcamp* api();
		const Bandcamp* api() const;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserURIs() override;
		
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
		struct GenerateLibraryResumeData {
			String fanId;
			
			Optional<time_t> mostRecentHiddenSave;
			Optional<time_t> mostRecentCollectionSave;
			Optional<time_t> mostRecentWishlistSave;
			
			String syncCurrentType;
			Optional<time_t> syncMostRecentSave;
			String syncLastToken;
			size_t syncOffset;
			
			Json toJson() const;
			static GenerateLibraryResumeData fromJson(const Json&);
			
			static String typeFromSyncIndex(size_t index);
			static Optional<size_t> syncIndexFromType(String type);
		};
		virtual LibraryItemGenerator generateLibrary(GenerateLibraryOptions options = GenerateLibraryOptions()) override;
		
		virtual BandcampPlaybackProvider* player() override;
		virtual const BandcampPlaybackProvider* player() const override;
		
		Track::Data createTrackData(BandcampTrack track, bool partial);
		Track::Data createTrackData(BandcampFan::CollectionTrack track);
		Artist::Data createArtistData(BandcampArtist artist, bool partial);
		Artist::Data createArtistData(BandcampFan::CollectionArtist artist);
		Album::Data createAlbumData(BandcampAlbum album, bool partial);
		Album::Data createAlbumData(BandcampFan::CollectionAlbum album);
		UserAccount::Data createUserData(BandcampFan fan);
		UserAccount::Data createUserData(BandcampFan::CollectionFan fan);
		
		struct URI {
			String provider;
			String url;
		};
		URI parseURI(String uri) const;
		
	protected:
		virtual Promise<Optional<BandcampIdentities>> fetchIdentity() override;
		virtual String getIdentityFilePath() const override;
		
	private:
		static time_t timeFromString(String time);
		
		URI parseURL(String url) const;
		String createURI(String type, String url) const;
		
		String idFromUserURL(String url) const;
		
		static MediaItem::Image createImage(BandcampImage image);
		ArrayList<$<Artist>> createArtists(String artistURL, String artistName, Optional<BandcampArtist> artist, bool partial);
		
		Bandcamp* bandcamp;
		BandcampPlaybackProvider* _player;
	};
}
