//
//  LastFM.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "LastFMAuth.hpp"
#include "LastFMError.hpp"
#include "LastFMTypes.hpp"
#include <soundhole/utils/HttpClient.hpp>

namespace sh {
	class LastFM {
	public:
		using APIConfig = LastFMAuth::APIConfig;
		
		static const APIConfig APICONFIG_LASTFM;
		
		struct Options {
			LastFMAuth::Options auth;
		};
		LastFM(Options options, APIConfig apiConfig = APICONFIG_LASTFM);
		~LastFM();
		
		Promise<Json> sendRequest(utils::HttpMethod httpMethod, String apiMethod, Map<String,String> params = {}, bool usesAuth = true, bool includeSignature = true);
		
		LastFMAuth* getAuth();
		const LastFMAuth* getAuth() const;
		
		Promise<bool> login();
		void logout();
		
		bool isLoggedIn() const;
		
		
		Promise<LastFMScrobbleResponse> scrobble(LastFMScrobbleRequest request);
		
		struct SearchArtistOptions {
			String artist;
			Optional<size_t> page;
			Optional<size_t> limit;
		};
		Promise<LastFMArtistSearchResults> searchArtist(SearchArtistOptions options);
		struct GetArtistInfoOptions {
			String artist;
			String mbid;
			String username;
			String lang;
			Optional<bool> autocorrect;
		};
		Promise<LastFMArtistInfo> getArtistInfo(GetArtistInfoOptions options);
		struct GetArtistTopItemsOptions {
			String artist;
			String mbid;
			Optional<bool> autocorrect;
			Optional<size_t> page;
			Optional<size_t> limit;
		};
		Promise<LastFMArtistItemsPage<LastFMTrackInfo>> getArtistTopTracks(GetArtistTopItemsOptions options);
		Promise<LastFMArtistItemsPage<LastFMArtistTopAlbum>> getArtistTopAlbums(GetArtistTopItemsOptions options);
		
		
		struct SearchTrackOptions {
			String track;
			String artist;
			Optional<size_t> page;
			Optional<size_t> limit;
		};
		Promise<LastFMTrackSearchResults> searchTrack(SearchTrackOptions options);
		struct GetTrackInfoOptions {
			String track;
			String artist;
			String mbid;
			String username;
			Optional<bool> autocorrect;
		};
		Promise<LastFMTrackInfo> getTrackInfo(GetTrackInfoOptions options);
		Promise<void> loveTrack(String track, String artist);
		Promise<void> unloveTrack(String track, String artist);
		
		
		struct SearchAlbumOptions {
			String album;
			String artist;
			Optional<size_t> page;
			Optional<size_t> limit;
		};
		Promise<LastFMAlbumSearchResults> searchAlbum(SearchAlbumOptions options);
		struct GetAlbumInfoOptions {
			String album;
			String artist;
			String mbid;
			String username;
			String lang;
			Optional<bool> autocorrect;
		};
		Promise<LastFMAlbumInfo> getAlbumInfo(GetAlbumInfoOptions options);
		
        
		Promise<LastFMUserInfo> getUserInfo(String user);
        Promise<LastFMUserInfo> getCurrentUserInfo();
		struct GetRecentTracksOptions {
			Optional<Date> from;
			Optional<Date> to;
			Optional<size_t> page;
			Optional<size_t> limit;
			Optional<bool> extended;
		};
		Promise<LastFMUserItemsPage<LastFMUserRecentTrack>> getUserRecentTracks(String user, GetRecentTracksOptions options);
		Promise<LastFMUserItemsPage<LastFMUserRecentTrack>> getCurrentUserRecentTracks(GetRecentTracksOptions options);
		
	private:
		LastFMAuth* auth;
	};
}
