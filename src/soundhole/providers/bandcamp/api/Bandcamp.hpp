//
//  Bandcamp.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/16/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/js/JSWrapClass.hpp>
#include "BandcampAuth.hpp"
#include "BandcampMediaTypes.hpp"

namespace sh {
	class Bandcamp: private JSWrapClass {
	public:
		struct Options {
			BandcampAuth::Options auth;
		};
		
		Bandcamp(const Bandcamp&) = delete;
		Bandcamp& operator=(const Bandcamp&) = delete;
		
		Bandcamp(Options options);
		~Bandcamp();
		
		BandcampAuth* getAuth();
		const BandcampAuth* getAuth() const;
		
		Promise<bool> login();
		void logout();
		bool isLoggedIn() const;
		
		Promise<BandcampIdentities> getMyIdentities();
		
		struct SearchOptions {
			size_t page = 0;
		};
		Promise<BandcampSearchResults> search(String query, SearchOptions options = SearchOptions{.page=0});
		
		Promise<BandcampTrack> getTrack(String url);
		Promise<BandcampAlbum> getAlbum(String url);
		Promise<BandcampArtist> getArtist(String url);
		Promise<BandcampFan> getFan(String url);
		
		struct GetFanSectionItemsOptions {
			String olderThanToken;
			Optional<size_t> count;
			
			#ifdef NAPI_MODULE
			Napi::Object toNapiObject(napi_env) const;
			#endif
		};
		Promise<BandcampFanSectionPage<BandcampFan::CollectionItemNode>> getFanCollectionItems(String fanURL, String fanId, GetFanSectionItemsOptions options);
		Promise<BandcampFanSectionPage<BandcampFan::CollectionItemNode>> getFanWishlistItems(String fanURL, String fanId, GetFanSectionItemsOptions options);
		Promise<BandcampFanSectionPage<BandcampFan::CollectionItemNode>> getFanHiddenItems(String fanURL, String fanId, GetFanSectionItemsOptions options);
		Promise<BandcampFanSectionPage<BandcampFan::FollowArtistNode>> getFanFollowingArtists(String fanURL, String fanId, GetFanSectionItemsOptions options);
		Promise<BandcampFanSectionPage<BandcampFan::FollowFanNode>> getFanFollowingFans(String fanURL, String fanId, GetFanSectionItemsOptions options);
		Promise<BandcampFanSectionPage<BandcampFan::FollowFanNode>> getFanFollowers(String fanURL, String fanId, GetFanSectionItemsOptions options);
		
		Promise<void> followFan(String fanURL);
		Promise<void> unfollowFan(String fanURL);
		
		Promise<void> followArtist(String artistURL);
		Promise<void> unfollowArtist(String artistURL);
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		void updateJSSession(napi_env env, Optional<BandcampSession> session);
		void queueUpdateJSSession(Optional<BandcampSession> session);
		void updateSessionFromJS(napi_env env);
		
		#ifdef NODE_API_MODULE
		template<typename Result>
		Promise<Result> performAsyncBandcampJSFunc(String funcName, Function<std::vector<napi_value>(napi_env)> createArgs, Function<Result(napi_env,Napi::Value)> mapper);
		#endif
		
		napi_ref jsRef;
		
		BandcampAuth* auth;
	};
}
