//
//  LastFMTypes.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	struct LastFMAPICredentials {
		String key;
		String secret;
	};


	struct LastFMSession {
		String name;
		bool subscriber;
		String key;
		
		static LastFMSession fromJson(const Json& json);
		Json toJson() const;
	};


	struct LastFMWebAuthResult {
		String username;
		String password;
		ArrayList<String> cookies;
	};


	struct LastFMScrobbleRequest {
		struct Item {
			String track;
			String artist;
			String album;
			String albumArtist;
			Date timestamp;
			Optional<bool> chosenByUser;
			Optional<double> duration;
			Optional<size_t> trackNumber;
			String mbid;
			
			void appendQueryItems(std::map<String,String>& queryItems, size_t index) const;
		};
		
		ArrayList<Item> items;
		
		std::map<String,String> toQueryItems() const;
	};


	struct LastFMScrobbleResponse {
		struct IgnoredMessage {
			String code;
			String text;
			
			static IgnoredMessage fromJson(const Json&);
		};
		
		struct Item {
			struct Property {
				Optional<String> text;
				bool corrected;
				
				static Property fromJson(const Json&);
			};
			
			IgnoredMessage ignoredMessage;
			
			Property track;
			Property artist;
			Property album;
			Property albumArtist;
			Date timestamp;
			
			static Item fromJson(const Json&);
		};
		
		size_t accepted;
		size_t ignored;
		
		ArrayList<Item> scrobbles;
		
		static LastFMScrobbleResponse fromJson(const Json&);
	};



    struct LastFMImage {
		String url;
        String size;
        
        static LastFMImage fromJson(const Json& json);
		Json toJson() const;
    };

	struct LastFMTag {
		String name;
		String url;
		
		static LastFMTag fromJson(const Json&);
	};

	struct LastFMLink {
		String text;
		String rel;
		String href;
		
		static LastFMLink fromJson(const Json&);
	};



	struct LastFMPartialArtistInfo {
		String name;
		String mbid;
		String url;
		Optional<ArrayList<LastFMImage>> image;
		
		Optional<size_t> listeners;
		Optional<bool> streamable;
		
		static LastFMPartialArtistInfo fromJson(const Json&);
	};



    struct LastFMUserInfo {
        String type;
        String url;
        String name;
        String realname;
        String country;
        String age;
        String gender;
        Optional<size_t> playCount;
        Optional<bool> subscriber;
        String playlists;
        String bootstrap;
        ArrayList<LastFMImage> image;
        String registeredTime;
        String registeredUnixTime;
        
        static LastFMUserInfo fromJson(const Json&);
		Json toJson() const;
    };



	struct LastFMArtistInfoRequest {
		String artist;
		String mbid;
		String username;
		String lang;
		Optional<bool> autocorrect;
	};

	struct LastFMArtistInfo {
		struct Stats {
			Optional<size_t> listeners;
			Optional<size_t> playCount;
			Optional<size_t> userPlayCount;
		};
		
		struct Bio {
			Optional<Date> published;
			String summary;
			String content;
			ArrayList<LastFMLink> links;
		};
		
		String mbid;
		String url;
		String name;
		ArrayList<LastFMImage> image;
		
		Optional<bool> streamable;
		Optional<bool> onTour;
		Stats stats;
		ArrayList<LastFMPartialArtistInfo> similarArtists;
		ArrayList<LastFMTag> tags;
		Bio bio;
		
		static LastFMArtistInfo fromJson(const Json&);
	};



	struct LastFMTrackInfoRequest {
		String track;
		String artist;
		String mbid;
		String username;
		Optional<bool> autocorrect;
	};

    struct LastFMTrackInfo {
        struct Album {
            String artist;
            String title;
            String url;
            ArrayList<LastFMImage> image;
            
            static Album fromJson(const Json&);
			static Optional<Album> maybeFromJson(const Json&);
        };
        
        struct Wiki {
            Optional<Date> published;
            String summary;
            String content;
            
            static Wiki fromJson(const Json&);
			static Optional<Wiki> maybeFromJson(const Json&);
        };
        
        String name;
		String mbid;
        String url;
        Optional<double> duration;
        Optional<bool> isStreamable;
        Optional<bool> isFullTrackStreamable;
        Optional<size_t> listeners;
        Optional<size_t> playCount;
        Optional<size_t> userPlayCount;
        Optional<bool> userLoved;
		LastFMPartialArtistInfo artist;
        Optional<Album> album;
        Optional<ArrayList<LastFMTag>> topTags;
        Optional<Wiki> wiki;
        
        static LastFMTrackInfo fromJson(const Json&);
    };



	struct LastFMAlbumInfoRequest {
		String album;
		String artist;
		String mbid;
		String username;
		String lang;
		Optional<bool> autocorrect;
	};

	struct LastFMAlbumInfo {
		struct Track {
			String url;
			String name;
			Optional<double> duration;
			Optional<size_t> rank;
			Optional<bool> isStreamable;
			Optional<bool> isFullTrackStreamable;
			LastFMPartialArtistInfo artist;
			
			static Track fromJson(const Json&);
		};
		
		String mbid;
		String url;
		String name;
		String artist;
		ArrayList<LastFMTag> tags;
		ArrayList<LastFMImage> image;
		Optional<size_t> listeners;
		Optional<size_t> playCount;
		Optional<size_t> userPlayCount;
		ArrayList<Track> tracks;
		
		static LastFMAlbumInfo fromJson(const Json&);
	};



	struct LastFMCorrectedArtist {
		String mbid;
		String name;
		String url;
		
		static LastFMCorrectedArtist fromJson(const Json&);
	};

	struct LastFMCorrectedTrack {
		String mbid;
		String name;
		String url;
		LastFMCorrectedArtist artist;
		
		static LastFMCorrectedTrack fromJson(const Json&);
	};


	struct LastFMTrackCorrectionResponse {
		struct Correction {
			LastFMCorrectedTrack track;
			Optional<size_t> index;
			Optional<bool> artistCorrected;
			Optional<bool> trackCorrected;
			
			static Correction fromJson(const Json&);
		};
		
		ArrayList<Correction> corrections;
		
		static LastFMTrackCorrectionResponse fromJson(const Json&);
	};

	struct LastFMArtistCorrectionResponse {
		struct Correction {
			LastFMCorrectedArtist artist;
			Optional<size_t> index;
			
			static Correction fromJson(const Json&);
		};
		
		ArrayList<Correction> corrections;
		
		static LastFMArtistCorrectionResponse fromJson(const Json&);
	};



	struct LastFMTrackSearchRequest {
		String track;
		String artist;
		Optional<size_t> page;
		Optional<size_t> limit;
	};

	struct LastFMTrackSearchResults {
		struct Item {
			String mbid;
			String name;
			String artist;
			String url;
			Optional<bool> streamable;
			String listeners;
			ArrayList<LastFMImage> image;
			
			static Item fromJson(const Json&);
		};
		
		size_t startPage;
		size_t startIndex;
		size_t itemsPerPage;
		size_t totalResults;
		
		ArrayList<Item> items;
		
		static LastFMTrackSearchResults fromJson(const Json&);
	};



	struct LastFMArtistSearchRequest {
		String artist;
		Optional<size_t> page;
		Optional<size_t> limit;
	};

	struct LastFMArtistSearchResults {
		struct Item {
			String mbid;
			String name;
			String url;
			String listeners;
			Optional<bool> streamable;
			ArrayList<LastFMImage> image;
			
			static Item fromJson(const Json&);
		};
		
		size_t startPage;
		size_t startIndex;
		size_t itemsPerPage;
		size_t totalResults;
		
		ArrayList<Item> items;
		
		static LastFMArtistSearchResults fromJson(const Json&);
	};



	struct LastFMAlbumSearchRequest {
		String album;
		String artist;
		Optional<size_t> page;
		Optional<size_t> limit;
	};

	struct LastFMAlbumSearchResults {
		struct Item {
			String mbid;
			String name;
			String artist;
			String url;
			ArrayList<LastFMImage> image;
			
			static Item fromJson(const Json&);
		};
		
		size_t startPage;
		size_t startIndex;
		size_t itemsPerPage;
		size_t totalResults;
		
		ArrayList<Item> items;
		Optional<bool> streamable;
		
		static LastFMAlbumSearchResults fromJson(const Json&);
	};



	struct LastFMArtistTopItemsRequest {
		String artist;
		String mbid;
		Optional<bool> autocorrect;
		Optional<size_t> page;
		Optional<size_t> limit;
		
		Map<String,String> toQueryParams() const;
	};


	struct LastFMArtistTopAlbum {
		String mbid;
		String name;
		String url;
		LastFMPartialArtistInfo artist;
		ArrayList<LastFMImage> image;
		Optional<size_t> playCount;
		
		static LastFMArtistTopAlbum fromJson(const Json&);
	};



	struct LastFMArtistTopItemsPageAttrs {
		String artist;
		size_t page;
		size_t perPage;
		size_t totalPages;
		size_t total;
		
		static LastFMArtistTopItemsPageAttrs fromJson(const Json&);
	};

	template<typename T>
	struct LastFMArtistTopItemsPage {
		ArrayList<T> items;
		LastFMArtistTopItemsPageAttrs attrs;
	};
}
