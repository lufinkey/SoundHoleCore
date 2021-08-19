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
#include <soundhole/utils/js/JSUtils.hpp>
#include "api/GoogleDriveStorageMediaTypes.hpp"

namespace sh {
	class GoogleDriveStorageProvider: public StorageProvider, public AuthedProviderIdentityStore<GoogleDriveStorageProviderUser>, private JSWrapClass {
		friend class GoogleDrivePlaylistMutatorDelegate;
	public:
		struct Options {
			String clientId;
			String redirectURL;
			String sessionPersistKey;
		};
		
		GoogleDriveStorageProvider(MediaItemBuilder* mediaItemBuilder, MediaProviderStash* mediaProviderStash, Options options);
		virtual ~GoogleDriveStorageProvider();
		
		String getWebAuthenticationURL(String codeChallenge) const;
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserURIs() override;
		
		virtual Promise<$<Playlist>> createPlaylist(String name, CreatePlaylistOptions options = CreatePlaylistOptions()) override;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) override;
		virtual Promise<void> deletePlaylist(String uri) override;
		virtual Promise<bool> isPlaylistEditable($<Playlist> playlist) override;
		
		virtual UserPlaylistsGenerator getUserPlaylists(String userURI) override;
		
		struct GenerateLibraryResumeData {
			struct Item {
				String uri;
				String addedAt;
				
				Json toJson() const;
				static Optional<Item> maybeFromJson(const Json&);
			};
			
			Optional<time_t> mostRecentPlaylistModification;
			Optional<time_t> mostRecentPlaylistFollow;
			Optional<time_t> mostRecentArtistFollow;
			Optional<time_t> mostRecentUserFollow;
			
			String syncCurrentType;
			String syncPageToken;
			Optional<time_t> syncMostRecentSave;
			Optional<size_t> syncLastItemOffset;
			Optional<Item> syncLastItem;
			
			Json toJson() const;
			static GenerateLibraryResumeData fromJson(const Json&);
		};
		virtual LibraryItemGenerator generateLibrary(GenerateLibraryOptions options) override;
		
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) override;
		
		virtual Promise<void> followPlaylists(ArrayList<NewFollowedItem> playlists) override;
		virtual Promise<void> unfollowPlaylists(ArrayList<String> playlistURIs) override;
		
		virtual Promise<void> followArtists(ArrayList<NewFollowedItem> artists) override;
		virtual Promise<void> unfollowArtists(ArrayList<String> artistURIs) override;
		
		virtual Promise<void> followUsers(ArrayList<NewFollowedItem> users) override;
		virtual Promise<void> unfollowUsers(ArrayList<String> userURIs) override;
		
	protected:
		virtual Promise<Optional<GoogleDriveStorageProviderUser>> fetchIdentity() override;
		virtual String getIdentityFilePath() const override;
		
		Promise<GoogleDrivePlaylistItemsPage> getPlaylistItems(String uri, size_t offset, size_t limit);
		Promise<GoogleDrivePlaylistItemsPage> insertPlaylistItems(String uri, size_t index, ArrayList<$<Track>> tracks);
		Promise<GoogleDrivePlaylistItemsPage> appendPlaylistItems(String uri, ArrayList<$<Track>> tracks);
		Promise<ArrayList<size_t>> deletePlaylistItems(String uri, ArrayList<String> itemIds);
		Promise<void> movePlaylistItems(String uri, size_t index, size_t count, size_t insertBefore);
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		struct PlaylistVersionID {
			Date modifiedAt;
		};
		PlaylistVersionID parsePlaylistVersionID(String versionIdString);
		
		struct GetMyPlaylistsOptions {
			String pageToken;
			Optional<size_t> pageSize;
			String orderBy;
		};
		Promise<GoogleDriveFilesPage<Playlist::Data>> getMyPlaylists(GetMyPlaylistsOptions options);
		Promise<GoogleSheetDBPage<FollowedItem>> getFollowedPlaylists(size_t offset, size_t limit);
		Promise<GoogleSheetDBPage<FollowedItem>> getFollowedArtists(size_t offset, size_t limit);
		Promise<GoogleSheetDBPage<FollowedItem>> getFollowedUsers(size_t offset, size_t limit);
		
		Promise<void> handleOAuthRedirect(std::map<String,String> params, String codeVerifier);
		Json sessionFromJS(napi_env env);
		void updateSessionFromJS(napi_env env);
		void loadSession();
		void saveSession();
		
		static time_t timeFromString(String dateString);
		
		#ifdef NODE_API_MODULE
		template<typename Result>
		Promise<Result> performAsyncJSAPIFunc(String funcName, Function<std::vector<napi_value>(napi_env)> createArgs, Function<Result(napi_env,Napi::Value)> mapper);
		#endif
		
		napi_ref jsRef;
		MediaItemBuilder* mediaItemBuilder;
		MediaProviderStash* mediaProviderStash;
		Options options;
		Json session;
	};
}
