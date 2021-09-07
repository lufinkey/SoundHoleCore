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
#include "api/Bandcamp.hpp"

namespace sh {
	class BandcampMediaProvider: public MediaProvider, public AuthedProviderIdentityStore<BandcampIdentities> {
		friend class BandcampAlbumMutatorDelegate;
	public:
		static constexpr auto NAME = "bandcamp";
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
			
			Optional<Date> mostRecentHiddenSave;
			Optional<Date> mostRecentCollectionSave;
			Optional<Date> mostRecentWishlistSave;
			
			String syncCurrentType;
			Optional<Date> syncMostRecentSave;
			String syncLastToken;
			size_t syncOffset;
			
			Json toJson() const;
			static GenerateLibraryResumeData fromJson(const Json&);
		};
		virtual LibraryItemGenerator generateLibrary(GenerateLibraryOptions options = GenerateLibraryOptions()) override;
		
		virtual bool canFollowArtists() const override;
		virtual Promise<void> followArtist(String artistURI) override;
		virtual Promise<void> unfollowArtist(String artistURI) override;
		
		virtual bool canFollowUsers() const override;
		virtual Promise<void> followUser(String userURI) override;
		virtual Promise<void> unfollowUser(String userURI) override;
		
		virtual Promise<void> saveTrack(String trackURI) override;
		virtual Promise<void> unsaveTrack(String trackURI) override;
		virtual Promise<void> saveAlbum(String albumURI) override;
		virtual Promise<void> unsaveAlbum(String albumURI) override;
		
		virtual bool canSavePlaylists() const override;
		virtual Promise<void> savePlaylist(String playlistURI) override;
		virtual Promise<void> unsavePlaylist(String playlistURI) override;
		
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
		URI parseURL(String url) const;
		String createURI(String type, String url) const;
		
		String idFromUserURL(String url) const;
		
		static MediaItem::Image createImage(BandcampImage image);
		ArrayList<$<Artist>> createArtists(String artistURL, String artistName, Optional<BandcampArtist> artist, bool partial);
		
		Bandcamp* bandcamp;
	};
}
