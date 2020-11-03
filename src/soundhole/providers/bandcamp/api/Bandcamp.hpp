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
		
		struct SearchOptions {
			size_t page = 0;
		};
		Promise<BandcampSearchResults> search(String query, SearchOptions options = SearchOptions{.page=0});
		
		Promise<BandcampTrack> getTrack(String url);
		Promise<BandcampAlbum> getAlbum(String url);
		Promise<BandcampArtist> getArtist(String url);
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		napi_ref jsRef;
		
		BandcampAuth* auth;
	};
}
