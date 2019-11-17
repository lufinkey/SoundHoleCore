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
		
		struct Options {
			String apiKey;
		};
		
		Youtube(const Youtube&) = delete;
		Youtube& operator=(const Youtube&) = delete;
		
		Youtube(Options options);
		~Youtube();
		
		Promise<Json> sendApiRequest(utils::HttpMethod method, String endpoint, std::map<String,String> query, Json body);
		
		struct SearchOptions {
			Optional<String> pageToken;
			ArrayList<MediaType> types;
		};
		Promise<YoutubePage<YoutubeSearchResult>> search(String query, SearchOptions options);
		
		Promise<YoutubeVideoInfo> getVideoInfo(String id);
		
		Promise<YoutubeVideo> getVideo(String id);
		Promise<YoutubeChannel> getChannel(String id);
		Promise<YoutubePlaylist> getPlaylist(String id);
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		Options options;
		napi_ref jsRef;
	};
}
