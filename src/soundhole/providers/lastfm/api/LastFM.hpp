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
		
		Promise<LastFMArtistSearchResults> searchArtist(LastFMArtistSearchRequest request);
		Promise<LastFMArtistInfo> getArtistInfo(LastFMArtistInfoRequest request);
		Promise<LastFMArtistTopItemsPage<LastFMTrackInfo>> getArtistTopTracks(LastFMArtistTopItemsRequest request);
		Promise<LastFMArtistTopItemsPage<LastFMArtistTopAlbum>> getArtistTopAlbums(LastFMArtistTopItemsRequest request);
		
		
		Promise<LastFMTrackSearchResults> searchTrack(LastFMTrackSearchRequest request);
		Promise<LastFMTrackInfo> getTrackInfo(LastFMTrackInfoRequest request);
		Promise<void> loveTrack(String track, String artist);
		Promise<void> unloveTrack(String track, String artist);
		
		
		Promise<LastFMAlbumSearchResults> searchAlbum(LastFMAlbumSearchRequest request);
		Promise<LastFMAlbumInfo> getAlbumInfo(LastFMAlbumInfoRequest request);
		
        
        Promise<LastFMUserInfo> getUserInfo(String user = String());
		
	private:
		LastFMAuth* auth;
	};
}
