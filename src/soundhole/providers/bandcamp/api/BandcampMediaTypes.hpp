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
		static Size Size_fromString(const String&);
		static String Size_toString(Size);
		
		String url;
		Size size;
		Optional<Dimensions> dimensions;
		
		Json toJson() const;
		
		static BandcampImage fromJson(const Json&);
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
		bool isLabel;
		ArrayList<BandcampImage> images;
		ArrayList<BandcampShow> shows;
		ArrayList<BandcampLink> links;
		Optional<ArrayList<BandcampAlbum>> albums;
		
		#ifdef NODE_API_MODULE
		static BandcampArtist fromNapiObject(Napi::Object);
		#endif
	};


	struct BandcampTrack {
		struct AudioSource {
			String type;
			String url;
			
			#ifdef NODE_API_MODULE
			static AudioSource fromNapiObject(Napi::Object);
			#endif
		};
		
		String url;
		String name;
		ArrayList<BandcampImage> images;
		String artistName;
		String artistURL;
		Optional<BandcampArtist> artist;
		String albumName;
		String albumURL;
		Optional<size_t> trackNumber;
		Optional<double> duration;
		Optional<ArrayList<String>> tags;
		Optional<String> description;
		Optional<ArrayList<AudioSource>> audioSources;
		Optional<bool> playable;
		
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
		Optional<ArrayList<String>> tags;
		Optional<String> description;
		Optional<size_t> numTracks;
		Optional<ArrayList<BandcampTrack>> tracks;
		
		#ifdef NODE_API_MODULE
		static BandcampAlbum fromNapiObject(Napi::Object);
		#endif
		
		static BandcampAlbum fromSingle(BandcampTrack track);
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


	struct BandcampIdentities {
		struct Fan {
			String id;
			String url;
			String name;
			Optional<ArrayList<BandcampImage>> images;
			
			Json toJson() const;
			
			static Fan fromJson(const Json&);
			static Optional<Fan> maybeFromJson(const Json&);
			#ifdef NODE_API_MODULE
			static Fan fromNapiObject(Napi::Object);
			static Optional<Fan> maybeFromNapiObject(Napi::Object);
			#endif
		};
		
		Optional<Fan> fan;
		
		Json toJson() const;
		
		static BandcampIdentities fromJson(const Json&);
		static Optional<BandcampIdentities> maybeFromJson(const Json&);
		#ifdef NODE_API_MODULE
		static BandcampIdentities fromNapiObject(Napi::Object);
		#endif
	};



	struct BandcampFan {
		template<typename ItemType>
		struct Section {
			size_t itemCount;
			Optional<size_t> batchSize;
			String lastToken;
			ArrayList<ItemType> items;
			
			#ifdef NODE_API_MODULE
			static Section<ItemType> fromNapiObject(Napi::Object);
			static Optional<Section<ItemType>> maybeFromNapiObject(Napi::Object);
			#endif
		};
		
		struct CollectionTrack {
			String url;
			String name;
			Optional<ArrayList<BandcampImage>> images;
			String artistName;
			String artistURL;
			Optional<double> duration;
			size_t trackNumber;
			String albumURL;
			Optional<String> albumName;
			Optional<String> albumSlug;
			
			#ifdef NODE_API_MODULE
			static CollectionTrack fromNapiObject(Napi::Object);
			#endif
		};
		
		struct CollectionAlbum {
			String url;
			String name;
			Optional<ArrayList<BandcampImage>> images;
			String artistName;
			String artistURL;
			
			#ifdef NODE_API_MODULE
			static CollectionAlbum fromNapiObject(Napi::Object);
			#endif
		};
		
		struct CollectionArtist {
			String url;
			String name;
			Optional<String> location;
			Optional<ArrayList<BandcampImage>> images;
			
			#ifdef NODE_API_MODULE
			static CollectionArtist fromNapiObject(Napi::Object);
			#endif
		};
		
		struct CollectionFan {
			String id;
			String url;
			String name;
			Optional<String> location;
			Optional<ArrayList<BandcampImage>> images;
			
			#ifdef NODE_API_MODULE
			static CollectionFan fromNapiObject(Napi::Object);
			#endif
		};
		
		struct CollectionItemNode {
			using ItemVariant = std::variant<CollectionTrack,CollectionAlbum>;
			
			String token;
			String dateAdded;
			ItemVariant item;
			
			Optional<CollectionTrack> trackItem() const;
			Optional<CollectionAlbum> albumItem() const;
			
			#ifdef NODE_API_MODULE
			static CollectionItemNode fromNapiObject(Napi::Object);
			#endif
		};
		
		template<typename ItemType>
		struct FollowItemNode {
			String token;
			String dateFollowed;
			ItemType item;
			
			#ifdef NODE_API_MODULE
			static FollowItemNode<ItemType> fromNapiObject(Napi::Object);
			#endif
		};
		
		using FollowArtistNode = FollowItemNode<CollectionArtist>;
		using FollowFanNode = FollowItemNode<CollectionFan>;
		
		
		String id;
		String url;
		String name;
		String description;
		Optional<ArrayList<BandcampImage>> images;
		
		Optional<Section<CollectionItemNode>> collection;
		Optional<Section<CollectionItemNode>> wishlist;
		Optional<Section<CollectionItemNode>> hiddenCollection;
		Optional<Section<FollowArtistNode>> followingArtists;
		Optional<Section<FollowFanNode>> followingFans;
		Optional<Section<FollowFanNode>> followers;
		
		#ifdef NODE_API_MODULE
		static BandcampFan fromNapiObject(Napi::Object);
		#endif
	};



	template<typename ItemType>
	struct BandcampFanSectionPage {
		bool hasMore;
		String lastToken;
		ArrayList<ItemType> items;
		
		#ifdef NODE_API_MODULE
		static BandcampFanSectionPage<ItemType> fromNapiObject(Napi::Object);
		#endif
	};
}

#include "BandcampMediaTypes.impl.hpp"
