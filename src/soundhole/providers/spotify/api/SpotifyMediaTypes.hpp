//
//  SpotifyMediaTypes.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	template<typename T>
	struct SpotifyPage;
	struct SpotifyPagePreview;
	struct SpotifySearchResults;
	struct SpotifyImage;
	struct SpotifyExternalURL;
	struct SpotifyExternalId;
	struct SpotifyUser;
	struct SpotifyArtist;
	struct SpotifyTrack;
	struct SpotifyAlbum;
	struct SpotifyPlaylist;


	template<typename T>
	struct SpotifyPage {
		String href;
		size_t limit;
		size_t offset;
		size_t total;
		String previous;
		String next;
		ArrayList<T> items;
		
		static SpotifyPage<T> fromJson(const Json&);
		static Optional<SpotifyPage<T>> maybeFromJson(const Json&);
		
		template<typename Transform>
		auto map(Transform transform) const;
	};


	struct SpotifyCursor {
		String after;
		
		static SpotifyCursor fromJson(const Json&);
	};

	template<typename T>
	struct SpotifyCursorPage {
		SpotifyCursor cursor;
		String href;
		size_t limit;
		size_t total;
		String next;
		ArrayList<T> items;
		
		static SpotifyCursorPage<T> fromJson(const Json&);
		static Optional<SpotifyCursorPage<T>> maybeFromJson(const Json&);
		
		template<typename Transform>
		auto map(Transform transform) const;
	};


	struct SpotifySearchResults {
		Optional<SpotifyPage<SpotifyTrack>> tracks;
		Optional<SpotifyPage<SpotifyAlbum>> albums;
		Optional<SpotifyPage<SpotifyArtist>> artists;
		Optional<SpotifyPage<SpotifyPlaylist>> playlists;
		
		static SpotifySearchResults fromJson(const Json&);
	};


	struct SpotifyImage {
		String url;
		size_t width;
		size_t height;
		
		Json toJson() const;
		static SpotifyImage fromJson(const Json&);
	};


	struct SpotifyCopyright {
		String text;
		String type;
		
		static SpotifyCopyright fromJson(const Json&);
	};


	struct SpotifyFollowers {
		String href;
		size_t total;
		
		Json toJson() const;
		static SpotifyFollowers fromJson(const Json&);
		static Optional<SpotifyFollowers> maybeFromJson(const Json&);
	};


	struct SpotifyVideoThumbnail {
		String url;
		
		static SpotifyVideoThumbnail fromJson(const Json&);
	};


	struct SpotifyUser {
		String type;
		String id;
		String uri;
		String href;
		Optional<String> displayName;
		Optional<ArrayList<SpotifyImage>> images;
		std::map<String,String> externalURLs;
		Optional<SpotifyFollowers> followers;
		
		Json toJson() const;
		static SpotifyUser fromJson(const Json&);
	};


	struct SpotifyArtist {
		String type;
		String id;
		String uri;
		String href;
		String name;
		std::map<String,String> externalURLs;
		
		Optional<ArrayList<SpotifyImage>> images;
		Optional<ArrayList<String>> genres;
		Optional<SpotifyFollowers> followers;
		Optional<size_t> popularity;
		
		static SpotifyArtist fromJson(const Json&);
	};


	struct SpotifyAlbum {
		String type;
		String albumType;
		String id;
		String uri;
		String href;
		String name;
		
		ArrayList<SpotifyArtist> artists;
		ArrayList<SpotifyImage> images;
		std::map<String,String> externalURLs;
		Optional<ArrayList<String>> availableMarkets;
		Optional<ArrayList<SpotifyCopyright>> copyrights;
		
		Optional<String> label;
		String releaseDate;
		String releaseDatePrecision;
		
		Optional<SpotifyPage<SpotifyTrack>> tracks;
		
		static SpotifyAlbum fromJson(const Json&);
	};


	struct SpotifyTrack {
		String type;
		String id;
		String uri;
		String href;
		String name;
		
		Optional<SpotifyAlbum> album;
		ArrayList<SpotifyArtist> artists;
		Optional<ArrayList<String>> availableMarkets;
		std::map<String,String> externalIds;
		std::map<String,String> externalURLs;
		
		String previewURL;
		size_t trackNumber;
		size_t discNumber;
		uint64_t durationMs;
		size_t popularity;
		bool isLocal;
		bool isExplicit;
		Optional<bool> isPlayable;
		
		static SpotifyTrack fromJson(const Json&);
	};


	struct SpotifyPlaylist {
		struct Item {
			String addedAt;
			Optional<SpotifyUser> addedBy;
			SpotifyVideoThumbnail videoThumbnail;
			SpotifyTrack track;
			
			static Item fromJson(const Json&);
		};
		
		struct AddResult {
			String snapshotId;
			
			static AddResult fromJson(const Json&);
		};
		
		struct MoveResult {
			String snapshotId;
			
			static MoveResult fromJson(const Json&);
		};
		
		struct RemoveResult {
			String snapshotId;
			
			static RemoveResult fromJson(const Json&);
		};
		
		String type;
		String id;
		String uri;
		String href;
		String name;
		SpotifyUser owner;
		Optional<String> description;
		String snapshotId;
		
		ArrayList<SpotifyImage> images;
		std::map<String,String> externalURLs;
		Optional<SpotifyFollowers> followers;
		SpotifyPage<SpotifyPlaylist::Item> tracks;
		
		Optional<bool> isPublic;
		bool isCollaborative;
		
		static SpotifyPlaylist fromJson(const Json&);
	};


	struct SpotifySavedTrack {
		String addedAt;
		SpotifyTrack track;
		
		static SpotifySavedTrack fromJson(const Json&);
	};

	struct SpotifySavedAlbum {
		String addedAt;
		SpotifyAlbum album;
		
		static SpotifySavedAlbum fromJson(const Json&);
	};
}


#include "SpotifyMediaTypes.impl.hpp"
