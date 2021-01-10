//
//  GoogleDriveStorageProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/storage/StorageProvider.hpp>
#include <soundhole/media/AuthedProviderIdentityStore.hpp>
#include <soundhole/utils/js/JSWrapClass.hpp>

namespace sh {
	struct GoogleDriveStorageProviderUser {
		String id;
		String kind;
		String displayName;
		String photoLink;
		String permissionId;
		String emailAddress;
		bool me;
		String baseFolderId;
		
		Json toJson() const;
		
		static GoogleDriveStorageProviderUser fromJson(const Json&);
		#ifdef NODE_API_MODULE
		static GoogleDriveStorageProviderUser fromNapiObject(Napi::Object);
		#endif
	};



	class GoogleDriveStorageProvider: public StorageProvider, public AuthedProviderIdentityStore<GoogleDriveStorageProviderUser>, private JSWrapClass {
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
		
	protected:
		virtual Promise<Optional<GoogleDriveStorageProviderUser>> fetchIdentity() override;
		virtual String getIdentityFilePath() const override;
		
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
