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
#include <soundhole/media/MediaProviderStash.hpp>
#include <soundhole/utils/js/JSWrapClass.hpp>
#include "api/GoogleDriveStorageMediaTypes.hpp"

namespace sh {
	class GoogleDriveStorageProvider: public StorageProvider, public AuthedProviderIdentityStore<GoogleDriveStorageProviderUser>, private JSWrapClass {
		friend class GoogleDrivePlaylistMutatorDelegate;
	public:
		struct AuthOptions {
			String clientId;
			String clientSecret;
			String redirectURL;
			String sessionPersistKey;
		};
		
		struct Options {
			MediaItemBuilder* mediaItemBuilder = nullptr;
			MediaProviderStash* mediaProviderStash = nullptr;
			AuthOptions auth;
		};
		
		GoogleDriveStorageProvider(Options options);
		virtual ~GoogleDriveStorageProvider();
		
		String getWebAuthenticationURL(String codeChallenge) const;
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserURIs() override;
		
		virtual bool canStorePlaylists() const override;
		virtual Promise<$<Playlist>> createPlaylist(String name, CreatePlaylistOptions options = CreatePlaylistOptions()) override;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) override;
		virtual Promise<void> deletePlaylist(String uri) override;
		
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) override;
		
	protected:
		virtual Promise<Optional<GoogleDriveStorageProviderUser>> fetchIdentity() override;
		virtual String getIdentityFilePath() const override;
		
		Promise<GoogleDrivePlaylistItemsPage> getPlaylistItems(String uri, size_t offset, size_t limit);
		Promise<GoogleDrivePlaylistItemsPage> insertPlaylistItems(String uri, size_t index, ArrayList<$<Track>> tracks);
		Promise<GoogleDrivePlaylistItemsPage> appendPlaylistItems(String uri, ArrayList<$<Track>> tracks);
		Promise<ArrayList<size_t>> deletePlaylistItems(String uri, ArrayList<String> itemIds);
		Promise<void> reorderPlaylistItems(String uri, size_t index, size_t count, size_t insertBefore);
		
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
		Json session;
	};
}
