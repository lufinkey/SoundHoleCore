//
//  Bandcamp.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/16/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class Bandcamp {
	public:
		Bandcamp();
		~Bandcamp();
		
		struct SearchOptions {
			size_t page = 0;
		};
		Promise<Json> search(String query, SearchOptions options = SearchOptions{.page=0});
		
		Promise<Json> getTrack(String url);
		Promise<Json> getAlbum(String url);
		Promise<Json> getArtist(String url);
		
	private:
		void initIfNeeded(napi_env env);
		void performJSOp(Function<void(napi_env)> work);
		
		#ifdef NODE_API_MODULE
		Napi::Object getJSAPI(napi_env env);
		Json jsonFromNAPIValue(napi_env env, napi_value value);
		Promise<Json> jsonPromiseFromNAPIValue(napi_env env, napi_value value);
		#endif
		
		napi_ref jsRef;
	};
}
