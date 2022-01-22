//
//  LastFMMediaProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/9/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include <soundhole/media/AuthedProviderIdentityStore.hpp>
#include <soundhole/media/Scrobbler.hpp>
#include "api/LastFM.hpp"

namespace sh {
	class LastFMMediaProvider: public MediaProvider, public AuthedProviderIdentityStore<LastFMUserInfo>, public Scrobbler {
	public:
		static constexpr auto NAME = "lastfm";
		
		using Options = LastFM::Options;
		
		LastFMMediaProvider(Options options);
		virtual ~LastFMMediaProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		LastFM* api();
		const LastFM* api() const;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserURIs() override;
		
		virtual Promise<Track::Data> getTrackData(String uri) override;
		virtual Promise<Artist::Data> getArtistData(String uri) override;
		virtual Promise<Album::Data> getAlbumData(String uri) override;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) override;
		virtual Promise<UserAccount::Data> getUserData(String uri) override;
		
		virtual Promise<ArrayList<$<Track>>> getArtistTopTracks(String artistURI) override;
		virtual ArtistAlbumsGenerator getArtistAlbums(String artistURI) override;
		virtual UserPlaylistsGenerator getUserPlaylists(String userURI) override;
		
		virtual Album::MutatorDelegate* createAlbumMutatorDelegate($<Album> album) override;
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) override;
		
		virtual bool hasLibrary() const override;
		virtual LibraryItemGenerator generateLibrary(GenerateLibraryOptions options = GenerateLibraryOptions()) override;
		
		virtual bool canFollowArtists() const override;
		virtual Promise<void> followArtist(String artistURI) override;
		virtual Promise<void> unfollowArtist(String artistURI) override;
		
		virtual bool canFollowUsers() const override;
		virtual Promise<void> followUser(String userURI) override;
		virtual Promise<void> unfollowUser(String userURI) override;
		
		virtual bool canSaveTracks() const override;
		virtual Promise<void> saveTrack(String trackURI) override;
		virtual Promise<void> unsaveTrack(String trackURI) override;
		
		virtual bool canSaveAlbums() const override;
		virtual Promise<void> saveAlbum(String albumURI) override;
		virtual Promise<void> unsaveAlbum(String albumURI) override;
		
		virtual bool canSavePlaylists() const override;
		virtual Promise<void> savePlaylist(String playlistURI) override;
		virtual Promise<void> unsavePlaylist(String playlistURI) override;
		
		Artist::Data createArtistData(LastFMArtistInfo);
		Artist::Data createArtistData(LastFMPartialArtistInfo);
		Artist::Data createArtistData(LastFMArtistSearchResults::Item);
		Track::Data createTrackData(LastFMTrackInfo);
		Track::Data createTrackData(LastFMTrackSearchResults::Item);
		Album::Data createAlbumData(LastFMAlbumInfo);
		Album::Data createAlbumData(LastFMArtistTopAlbum);
		UserAccount::Data createUserAccountData(LastFMUserInfo);
		Scrobble::Data createScrobbleData(LastFMUserRecentTrack);
		static MediaItem::Image createImage(LastFMImage);
		
		struct URI {
			String provider;
			String type;
			String id;
			
			String toString() const;
		};
		URI parseURI(const String&) const;
		String createURI(const String& type, const String& id);
		
		URI parseArtistURL(const String&) const;
		URI parseTrackURL(const String&) const;
		URI parseAlbumURL(const String&) const;
		URI parseUserURL(const String&) const;
		
		virtual size_t maxScrobblesPerRequest() const override;
		virtual Promise<ArrayList<Scrobble::Response>> scrobble(ArrayList<$<Scrobble>> scrobbles) override;
		
		virtual Promise<void> loveTrack($<Track>) override;
		virtual Promise<void> unloveTrack($<Track>) override;
		
		virtual size_t maxFetchedScrobblesPerRequest() const override;
		virtual Promise<ItemsPage<$<Scrobble>>> getScrobbles(GetScrobblesOptions options) override;
		virtual Promise<ItemsPage<$<Scrobble>>> getUserScrobbles(String user, GetScrobblesOptions options) override;
		
	protected:
		virtual Promise<Optional<LastFMUserInfo>> fetchIdentity() override;
		virtual String getIdentityFilePath() const override;
		
	private:
		static Scrobble::IgnoredReason::Code ignoredScrobbleCodeFromString(const String&);
		
		LastFM* lastfm;
	};
}
