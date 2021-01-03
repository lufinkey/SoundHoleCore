//
//  GoogleDriveStorageProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/StorageProvider.hpp>
#include <soundhole/utils/js/JSWrapClass.hpp>
#include "api/GoogleDriveMediaTypes.hpp"

namespace sh {
	class GoogleDriveStorageProvider: public StorageProvider, private JSWrapClass {
	public:
		struct Options {
			String clientId;
			String clientSecret;
			String redirectURL;
			String sessionPersistKey;
		};
		
		GoogleDriveStorageProvider(Options options);
		virtual ~GoogleDriveStorageProvider();
		
		String getWebAuthenticationURL(String codeChallenge) const;
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserIds() override;
		
		virtual bool canStorePlaylists() const override;
		virtual Promise<Playlist> createPlaylist(String name, CreatePlaylistOptions options = CreatePlaylistOptions()) override;
		virtual Promise<Playlist> getPlaylist(String id) override;
		virtual Promise<void> deletePlaylist(String id) override;
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		Promise<void> handleOAuthRedirect(std::map<String,String> params, String codeVerifier);
		Json sessionFromJS(napi_env env);
		void updateSessionFromJS(napi_env env);
		
		#ifdef NODE_API_MODULE
		template<typename Result>
		Promise<Result> performAsyncJSAPIFunc(String funcName, Function<std::vector<napi_value>(napi_env)> createArgs, Function<Result(napi_env,Napi::Value)> mapper);
		#endif
		
		napi_ref jsRef;
		Options options;
		Json credentials;
	};
}
