//
//  BandcampMediaTypes.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/21/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	struct BandcampImage;
	struct BandcampSearchResults;
	struct BandcampArtist;
	struct BandcampTrack;
	struct BandcampAlbum;
	struct BandcampShow;
	struct BandcampLink;


	struct BandcampImage {
		struct Dimensions {
			size_t width;
			size_t height;
		};
		enum class Size {
			SMALL,
			MEDIUM,
			LARGE
		};
		
		String url;
		Size size;
		Optional<Dimensions> dimensions;
		
		#ifdef NODE_API_MODULE
		static BandcampImage fromNapiObject(Napi::Object);
		#endif
	};


	struct BandcampSearchResults {
		struct Item {
			enum class Type {
				TRACK,
				ARTIST,
				ALBUM,
				LABEL,
				FAN,
				UNKNOWN
			};
			
			Type type;
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
		ArrayList<Item> items;
		
		#ifdef NODE_API_MODULE
		static BandcampSearchResults fromNapiObject(Napi::Object);
		#endif
	};


	struct BandcampArtist {
		String url;
		String name;
		String location;
		String description;
		ArrayList<BandcampImage> images;
		ArrayList<BandcampShow> shows;
		ArrayList<BandcampLink> links;
		Optional<ArrayList<BandcampAlbum>> albums;
		bool isLabel;
		
		#ifdef NODE_API_MODULE
		static BandcampArtist fromNapiObject(Napi::Object);
		#endif
	};


	struct BandcampTrack {
		String url;
		String name;
		ArrayList<BandcampImage> images;
		String artistName;
		String artistURL;
		Optional<BandcampArtist> artist;
		String albumName;
		String albumURL;
		Optional<String> audioURL;
		Optional<bool> playable;
		Optional<double> duration;
		Optional<ArrayList<String>> tags;
		Optional<String> description;
		
		#ifdef NODE_API_MODULE
		static BandcampTrack fromNapiObject(Napi::Object);
		#endif
	};


	struct BandcampAlbum {
		String url;
		String name;
		ArrayList<BandcampImage> images;
		String artistName;
		String artistURL;
		Optional<BandcampArtist> artist;
		Optional<ArrayList<BandcampTrack>> tracks;
		Optional<size_t> numTracks;
		Optional<ArrayList<String>> tags;
		Optional<String> description;
		
		#ifdef NODE_API_MODULE
		static BandcampAlbum fromNapiObject(Napi::Object);
		#endif
	};


	struct BandcampShow {
		String date;
		String url;
		String venueName;
		String location;
		
		#ifdef NODE_API_MODULE
		static BandcampShow fromNapiObject(Napi::Object);
		#endif
	};


	struct BandcampLink {
		String name;
		String url;
		
		#ifdef NODE_API_MODULE
		static BandcampLink fromNapiObject(Napi::Object);
		#endif
	};
}
