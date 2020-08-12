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
		
		Promise<YoutubeVideoInfo> getVideoInfo(String id);
		Promise<YoutubeVideo> getVideo(String id);
		
		struct GetChannelOptions {
			ArrayList<ResultPart> parts = { ResultPart::ID, ResultPart::SNIPPET };
		};
		Promise<YoutubeChannel> getChannel(String id, GetChannelOptions options = GetChannelOptions{.parts={ResultPart::ID,ResultPart::SNIPPET}});
		struct GetChannelPlaylistsOptions {
			Optional<size_t> maxResults;
			String pageToken;
		};
		Promise<YoutubePage<YoutubePlaylist>> getChannelPlaylists(String id, GetChannelPlaylistsOptions options = GetChannelPlaylistsOptions());
		
		Promise<YoutubePlaylist> getPlaylist(String id);
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
			String note;
			String startAt;
			String endAt;
		};
		Promise<YoutubePlaylistItem> updatePlaylistItem(String playlistItemId, UpdatePlaylistItemOptions options);
		
		Promise<void> deletePlaylistItem(String playlistItemId);
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		Options options;
		napi_ref jsRef;
	};
}
