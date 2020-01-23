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
		
		template<typename U>
		YoutubePage<U> map(Function<U(const T&)> mapper) const;
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


	struct YoutubePlaylistItem {
		struct Snippet {
			struct ResourceId {
				String kind;
				String videoId;
				
				static ResourceId fromJson(const Json&);
			};
			
			String channelId;
			String channelTitle;
			String title;
			String description;
			String publishedAt;
			ArrayList<YoutubeImage> thumbnails;
			Optional<YoutubeImage> thumbnail(YoutubeImage::Size size) const;
			String playlistId;
			size_t position;
			ResourceId resourceId;
			
			static Snippet fromJson(const Json&);
		};
		
		String kind;
		String etag;
		String id;
		Snippet snippet;
		
		static YoutubePlaylistItem fromJson(const Json&);
	};



	struct YoutubeVideoInfo {
		struct Image {
			String url;
			size_t width;
			size_t height;
			
			#ifdef NODE_API_MODULE
			static Image fromNapiObject(Napi::Object);
			#endif
		};
		
		struct Format {
			String projectionType;
			String clen;
			String init;
			String fps;
			String index;
			
			String itag;
			String url;
			String type;
			
			String quality;
			String qualityLabel;
			String size;
			
			String sp;
			String s;
			String container;
			String resolution;
			String encoding;
			String profile;
			String bitrate;
			String audioEncoding;
			Optional<size_t> audioBitrate;
			
			bool live;
			bool isHLS;
			bool isDashMPD;
			
			#ifdef NODE_API_MODULE
			static Format fromNapiObject(Napi::Object);
			#endif
			
			enum class MediaTypeFilter {
				AUDIO_AND_VIDEO,
				VIDEO,
				VIDEO_ONLY,
				AUDIO,
				AUDIO_ONLY
			};
			static ArrayList<Format> filterFormats(const ArrayList<Format>& formats, MediaTypeFilter filter);
		};
		
		struct VideoDetails {
			struct Thumbnail {
				ArrayList<Image> thumbnails;
				
				#ifdef NODE_API_MODULE
				static Thumbnail fromNapiObject(Napi::Object);
				static Optional<Thumbnail> maybeFromNapiObject(Napi::Object);
				#endif
			};
			
			String videoId;
			String title;
			String lengthSeconds;
			ArrayList<String> keywords;
			String channelId;
			bool isCrawlable;
			Optional<Thumbnail> thumbnail;
			String viewCount;
			String author;
			bool isLiveContent;
			
			#ifdef NODE_API_MODULE
			static VideoDetails fromNapiObject(Napi::Object);
			static Optional<VideoDetails> maybeFromNapiObject(Napi::Object);
			#endif
		};
		
		struct PlayerResponse {
			Optional<VideoDetails> videoDetails;
			
			#ifdef NODE_API_MODULE
			static PlayerResponse fromNapiObject(Napi::Object);
			static Optional<PlayerResponse> maybeFromNapiObject(Napi::Object);
			#endif
		};
		
		struct Media {
			String categoryURL;
			String category;
			String song;
			String artistURL;
			String artist;
			String licensedToYoutubeBy;
			
			#ifdef NODE_API_MODULE
			static Media fromNapiObject(Napi::Object);
			static Optional<Media> maybeFromNapiObject(Napi::Object);
			#endif
		};
		
		struct Author {
			String id;
			String name;
			String avatar;
			bool verified;
			String user;
			String channelURL;
			String userURL;
			
			#ifdef NODE_API_MODULE
			static Author fromNapiObject(Napi::Object);
			static Optional<Author> maybeFromNapiObject(Napi::Object);
			#endif
		};
		
		String videoId;
		String thumbnailURL;
		String title;
		ArrayList<Format> formats;
		double published;
		String description;
		Optional<Media> media;
		Optional<Author> author;
		Optional<PlayerResponse> playerResponse;
		
		#ifdef NODE_API_MODULE
		static YoutubeVideoInfo fromNapiObject(Napi::Object);
		#endif
		
		struct ChooseAudioFormatOptions {
			Optional<size_t> bitrate;
		};
		Optional<Format> chooseAudioFormat(ChooseAudioFormatOptions options = {}) const;
	};
}

#include "YoutubeMediaTypes.impl.hpp"
