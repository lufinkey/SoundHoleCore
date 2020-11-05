//
//  Youtube.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/js/JSWrapClass.hpp>
#include <soundhole/utils/HttpClient.hpp>
#include "YoutubeMediaTypes.hpp"

namespace sh {
	class Youtube: private JSWrapClass {
	public:
		enum class MediaType {
			VIDEO,
			CHANNEL,
			PLAYLIST
		};
		static String MediaType_toString(MediaType);
		
		enum class ResultPart {
			ID,
			SNIPPET,
			CONTENT_DETAILS
		};
		static String ResultPart_toString(ResultPart);
		
		enum class SearchOrder {
			RELEVANCE,
			DATE,
			RATING,
			TITLE,
			VIDEO_COUNT,
			VIEW_COUNT
		};
		static String SearchOrder_toString(SearchOrder);
		
		struct Options {
			String apiKey;
		};
		
		Youtube(const Youtube&) = delete;
		Youtube& operator=(const Youtube&) = delete;
		
		Youtube(Options options);
		~Youtube();
		
		Promise<Json> sendApiRequest(utils::HttpMethod method, String endpoint, std::map<String,String> query, Json body);
		
		
		
		struct SearchOptions {
			ArrayList<MediaType> types;
			Optional<size_t> maxResults;
			String pageToken;
			String channelId;
			Optional<SearchOrder> order;
		};
		Promise<YoutubePage<YoutubeSearchResult>> search(String query, SearchOptions options);
		
		
		
		Promise<YoutubeVideoInfo> getVideoInfo(String videoId);
		Promise<YoutubeVideo> getVideo(String videoId);
		
		
		
		struct GetChannelsOptions {
			ArrayList<String> ids;
			String forUsername;
			Optional<bool> managedByMe;
			Optional<bool> mine;
		};
		Promise<YoutubePage<YoutubeChannel>> getChannels(GetChannelsOptions options);
		Promise<YoutubeChannel> getChannel(String channelId);
		struct GetChannelPlaylistsOptions {
			Optional<size_t> maxResults;
			String pageToken;
		};
		Promise<YoutubePage<YoutubePlaylist>> getChannelPlaylists(String channelId, GetChannelPlaylistsOptions options = GetChannelPlaylistsOptions());
		
		
		
		struct GetChannelSectionsOptions {
			ArrayList<String> ids;
			String channelId;
			Optional<bool> mine;
		};
		Promise<YoutubeItemList<YoutubeChannelSection>> getChannelSections(GetChannelSectionsOptions options);
		struct InsertChannelSectionOptions {
			String type;
			String style;
			Optional<String> title;
			Optional<size_t> position;
			String defaultLanguage;
			ArrayList<String> playlists;
			ArrayList<String> channels;
			YoutubeChannelSection::Localizations localizations;
			Optional<YoutubeChannelSection::Targeting> targeting;
		};
		Promise<YoutubeChannelSection> insertChannelSection(InsertChannelSectionOptions options);
		struct UpdateChannelSectionOptions {
			String type;
			String style;
			Optional<String> title;
			Optional<size_t> position;
			Optional<String> defaultLanguage;
			Optional<ArrayList<String>> playlists;
			Optional<ArrayList<String>> channels;
			Optional<YoutubeChannelSection::Localizations> localizations;
			Optional<YoutubeChannelSection::Targeting> targeting;
		};
		Promise<YoutubeChannelSection> updateChannelSection(String channelSectionId, UpdateChannelSectionOptions options);
		Promise<void> deleteChannelSection(String channelSectionId);
		
		
		
		struct GetPlaylistsOptions {
			ArrayList<String> ids;
			String channelId;
			Optional<bool> mine;
			Optional<size_t> maxResults;
			String pageToken;
		};
		Promise<YoutubePage<YoutubePlaylist>> getPlaylists(GetPlaylistsOptions options);
		Promise<YoutubePlaylist> getPlaylist(String id);
		struct CreatePlaylistOptions {
			String description;
			String privacyStatus;
			ArrayList<String> tags;
			String defaultLanguage;
			YoutubePlaylist::Localizations localizations;
		};
		Promise<YoutubePlaylist> createPlaylist(String title, CreatePlaylistOptions options = CreatePlaylistOptions());
		struct UpdatePlaylistOptions {
			Optional<String> title;
			Optional<String> description;
			Optional<String> defaultLanguage;
			Optional<ArrayList<String>> tags;
			Optional<String> privacyStatus;
			Optional<YoutubePlaylist::Localizations> localizations;
		};
		Promise<YoutubePlaylist> updatePlaylist(String playlistId, UpdatePlaylistOptions options);
		Promise<void> deletePlaylist(String playlistId);
		
		
		
		struct GetPlaylistItemsOptions {
			Optional<size_t> maxResults;
			String pageToken;
			String videoId;
		};
		Promise<YoutubePage<YoutubePlaylistItem>> getPlaylistItems(String id, GetPlaylistItemsOptions options);
		struct InsertPlaylistItemOptions {
			Optional<size_t> position;
			String note;
			String startAt;
			String endAt;
		};
		Promise<YoutubePlaylistItem> insertPlaylistItem(String playlistId, String resourceId, InsertPlaylistItemOptions options = InsertPlaylistItemOptions());
		struct UpdatePlaylistItemOptions {
			String playlistId;
			String resourceId;
			Optional<size_t> position;
			Optional<String> note;
			Optional<String> startAt;
			Optional<String> endAt;
		};
		Promise<YoutubePlaylistItem> updatePlaylistItem(String playlistItemId, UpdatePlaylistItemOptions options);
		Promise<void> deletePlaylistItem(String playlistItemId);
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		napi_ref jsRef;
		Options options;
	};
}
