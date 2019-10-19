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

namespace sh {
	class Youtube: private JSWrapClass {
	public:
		enum class MediaType {
			VIDEO,
			CHANNEL,
			PLAYLIST
		};
		
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
		Promise<Json> search(String query, SearchOptions options);
		
		Promise<Json> getVideoInfo(String id);
		
		Promise<Json> getVideo(String id);
		Promise<Json> getChannel(String id);
		Promise<Json> getPlaylist(String id);
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		Options options;
		napi_ref jsRef;
	};
}
