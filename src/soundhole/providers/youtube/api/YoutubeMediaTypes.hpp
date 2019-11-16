//
//  YoutubeMediaTypes.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/29/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	struct YoutubeImage;
	template<typename T>
	struct YoutubePage;
	struct YoutubeSearchResult;
	struct YoutubeVideo;
	struct YoutubeChannel;
	struct YoutubePlaylist;


	struct YoutubeImage {
		enum class Size {
			DEFAULT,
			MEDIUM,
			HIGH,
			STANDARD,
			MAXRES
		};
		static Size Size_fromString(const std::string& str);
		
		struct Dimensions {
			size_t width;
			size_t height;
		};
		
		String url;
		Size size;
		Optional<Dimensions> dimensions;
		
		static ArrayList<YoutubeImage> arrayFromJson(const Json&);
	};


	template<typename T>
	struct YoutubePage {
		struct PageInfo {
			size_t totalResults;
			size_t resultsPerPage;
			
			static PageInfo fromJson(const Json&);
		};
		
		String kind;
		String etag;
		String prevPageToken;
		String nextPageToken;
		String regionCode;
		PageInfo pageInfo;
		ArrayList<T> items;
		
		static YoutubePage<T> fromJson(const Json&);
	};


	struct YoutubeSearchResult {
		struct Id {
			String kind;
			Optional<String> videoId;
			Optional<String> channelId;
			Optional<String> playlistId;
			
			static Id fromJson(const Json&);
		};
		
		struct Snippet {
			String channelId;
			String channelTitle;
			String title;
			String description;
			String publishedAt;
			ArrayList<YoutubeImage> thumbnails;
			Optional<YoutubeImage> thumbnail(YoutubeImage::Size size) const;
			String liveBroadcastContent;
			
			static Snippet fromJson(const Json&);
		};
		
		String kind;
		String etag;
		Id id;
		Snippet snippet;
		
		static YoutubeSearchResult fromJson(const Json&);
	};


	struct YoutubeVideo {
		struct Snippet {
			struct Localized {
				String title;
				String description;
				
				static Localized fromJson(const Json&);
			};
			
			String channelId;
			String channelTitle;
			String title;
			String description;
			String publishedAt;
			ArrayList<YoutubeImage> thumbnails;
			Optional<YoutubeImage> thumbnail(YoutubeImage::Size size) const;
			ArrayList<String> tags;
			String categoryId;
			String liveBroadcastContent;
			String defaultLanguage;
			Localized localized;
			String defaultAudioLanguage;
			
			static Snippet fromJson(const Json&);
		};
		
		String kind;
		String etag;
		String id;
		Snippet snippet;
		
		static YoutubeVideo fromJson(const Json&);
	};


	struct YoutubeChannel {
		struct Snippet {
			struct Localized {
				String title;
				String description;
				
				static Localized fromJson(const Json&);
			};
			
			String title;
			String description;
			String customURL;
			String publishedAt;
			ArrayList<YoutubeImage> thumbnails;
			Optional<YoutubeImage> thumbnail(YoutubeImage::Size size) const;
			String defaultLanguage;
			Localized localized;
			String country;
			
			static Snippet fromJson(const Json&);
		};
		
		String kind;
		String etag;
		String id;
		Snippet snippet;
		
		static YoutubeChannel fromJson(const Json&);
	};


	struct YoutubePlaylist {
		struct Snippet {
			struct Localized {
				String title;
				String description;
				
				static Localized fromJson(const Json&);
			};
			
			String channelId;
			String channelTitle;
			String title;
			String description;
			String publishedAt;
			ArrayList<YoutubeImage> thumbnails;
			Optional<YoutubeImage> thumbnail(YoutubeImage::Size size) const;
			ArrayList<String> tags;
			String defaultLanguage;
			Localized localized;
			
			static Snippet fromJson(const Json&);
		};
		
		String kind;
		String etag;
		String id;
		Snippet snippet;
		
		static YoutubePlaylist fromJson(const Json&);
	};
}

#include "YoutubeMediaTypes.impl.hpp"
