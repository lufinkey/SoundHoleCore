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

namespace sh {
	class Bandcamp: private JSWrapClass {
	public:
		enum class MediaType {
			TRACK,
			ARTIST,
			ALBUM,
			LABEL,
			FAN,
			UNKNOWN
		};
		
		struct SearchResults {
			struct Item {
				MediaType type;
				String name;
				String url;
				String imageURL;
				ArrayList<String> tags;
				String genre;
				String releaseDate;
				
				String artistName;
				String artistURL;
				
				String albumName;
				String albumURL;
				
				String location;
				
				Optional<size_t> numTracks;
				Optional<size_t> numMinutes;
			};
			
			String prevURL;
			String nextURL;
			LinkedList<Item> items;
		};
		
		Bandcamp(const Bandcamp&) = delete;
		Bandcamp& operator=(const Bandcamp&) = delete;
		
		Bandcamp();
		~Bandcamp();
		
		struct SearchOptions {
			size_t page = 0;
		};
		Promise<SearchResults> search(String query, SearchOptions options = SearchOptions{.page=0});
		
		Promise<Json> getTrack(String url);
		Promise<Json> getAlbum(String url);
		Promise<Json> getArtist(String url);
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		SearchResults searchResultsFromJson(Json json) const;
		
		napi_ref jsRef;
	};
}
