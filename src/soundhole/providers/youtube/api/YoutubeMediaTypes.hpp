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


	struct YoutubeLocalization {
		String title;
		String description;
		
		static YoutubeLocalization fromJson(const Json&);
		Json toJson() const;
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
		
		struct ContentDetails {
			struct RelatedPlaylists {
				String likes;
				String favorites;
				String uploads;
				String watchHistory;
				String watchLater;
				
				static RelatedPlaylists fromJson(const Json&);
			};
			
			RelatedPlaylists relatedPlaylists;
			
			static ContentDetails fromJson(const Json&);
		};
		
		String kind;
		String etag;
		String id;
		Snippet snippet;
		ContentDetails contentDetails;
		
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
		
		struct Status {
			String privacyStatus;
			
			static Status fromJson(const Json&);
		};
		
		struct ContentDetails {
			size_t itemCount;
			
			static ContentDetails fromJson(const Json&);
		};
		
		struct Player {
			String embedHtml;
			
			static Player fromJson(const Json&);
		};
		
		using Localizations = std::map<String,YoutubeLocalization>;
		
		String kind;
		String etag;
		String id;
		Snippet snippet;
		Status status;
		ContentDetails contentDetails;
		Player player;
		Localizations localizations;
		Optional<YoutubeLocalization> localization(const String& key) const;
		
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
		
		struct ContentDetails {
			String videoId;
			String startAt;
			String endAt;
			String note;
			String videoPublishedAt;
			
			static ContentDetails fromJson(const Json&);
		};
		
		struct Status {
			String privacyStatus;
			
			static Status fromJson(const Json&);
		};
		
		String kind;
		String etag;
		String id;
		Snippet snippet;
		ContentDetails contentDetails;
		Status status;
		
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
			String itag;
			String url;
			String mimeType;
			String bitrate;
			Optional<double> audioBitrate;
			Optional<size_t> width;
			Optional<size_t> height;
			String container;
			bool hasVideo;
			bool hasAudio;
			String codecs;
			String audioCodec;
			String videoCodec;
			String quality;
			String qualityLabel;
			String audioQuality;
			
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
				#endif
			};
			
			String videoId;
			String title;
			String shortDescription;
			String lengthSeconds;
			ArrayList<String> keywords;
			String channelId;
			bool isCrawlable;
			Thumbnail thumbnail;
			String viewCount;
			String author;
			bool isLiveContent;
			
			#ifdef NODE_API_MODULE
			static VideoDetails fromNapiObject(Napi::Object);
			static Optional<VideoDetails> maybeFromNapiObject(Napi::Object);
			#endif
		};
		
		struct PlayerResponse {
			VideoDetails videoDetails;
			
			#ifdef NODE_API_MODULE
			static PlayerResponse fromNapiObject(Napi::Object);
			static Optional<PlayerResponse> maybeFromNapiObject(Napi::Object);
			#endif
		};
		
		struct Media {
			String image;
			String categoryURL;
			String category;
			String song;
			Optional<size_t> year;
			String artist;
			String artistURL;
			String game;
			String gameURL;
			String licensedBy;
			
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
			String externalChannelURL;
			String userURL;
			size_t subscriberCount;
			
			#ifdef NODE_API_MODULE
			static Author fromNapiObject(Napi::Object);
			static Optional<Author> maybeFromNapiObject(Napi::Object);
			#endif
		};
		
		String thumbnailURL;
		ArrayList<Format> formats;
		Optional<Media> media;
		Optional<Author> author;
		PlayerResponse playerResponse;
		
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
