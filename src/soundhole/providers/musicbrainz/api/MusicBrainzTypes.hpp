//
//  MusicBrainzTypes.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/1/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	struct MusicBrainzPeriod;
	struct MusicBrainzArea;
	struct MusicBrainzAlias;
	struct MusicBrainzRelation;
	struct MusicBrainzArtist;
	struct MusicBrainzArtistCredit;
	struct MusicBrainzReleaseGroup;
	struct MusicBrainzRelease;
	struct MusicBrainzReleaseEvent;
	struct MusicBrainzRecording;
	struct MusicBrainzTrack;
	struct MusicBrainzMedia;
	struct MusicBrainzCoverArtArchive;

	struct MusicBrainzPeriod {
		String begin;
		String end;
		Optional<bool> ended;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzPeriod fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzURL {
		String id;
		String resource;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzURL fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzArea {
		String id;
		String typeId;
		String type;
		String name;
		String sortName;
		String disambiguation;
		Optional<ArrayList<String>> iso_3166_1_codes;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzArea fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzAlias {
		String name;
		String sortName;
		String type;
		String locale;
		Optional<bool> primary;
		Optional<bool> ended;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzAlias fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzRelation {
		using ItemVariant = std::variant<
			std::nullptr_t,
			std::shared_ptr<MusicBrainzURL>,
			std::shared_ptr<MusicBrainzRelease>,
			std::shared_ptr<MusicBrainzArtist>>;
		
		String targetType;
		String typeId;
		String type;
		String direction;
		String targetCredit;
		String sourceCredit;
		String begin;
		String end;
		Optional<bool> ended;
		
		ItemVariant item;
		
		std::shared_ptr<MusicBrainzURL> url();
		std::shared_ptr<const MusicBrainzURL> url() const;
		
		std::shared_ptr<MusicBrainzRelease> release();
		std::shared_ptr<const MusicBrainzRelease> release() const;
		
		std::shared_ptr<MusicBrainzArtist> artist();
		std::shared_ptr<const MusicBrainzArtist> artist() const;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzRelation fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzArtist {
		String id;
		String name;
		String sortName;
		String disambiguation;
		String country;
		Optional<ArrayList<MusicBrainzAlias>> aliases;
		Optional<MusicBrainzArea> area;
		Optional<MusicBrainzArea> beginArea;
		Optional<MusicBrainzArea> endArea;
		Optional<MusicBrainzPeriod> lifeSpan;
		Optional<ArrayList<MusicBrainzRelation>> relations;
		Optional<ArrayList<MusicBrainzRelease>> releases;
		Optional<ArrayList<MusicBrainzReleaseGroup>> releaseGroups;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzArtist fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzArtistCredit {
		MusicBrainzArtist artist;
		String joinphrase;
		String name;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzArtistCredit fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzCoverArtArchive {
		size_t count;
		Optional<bool> front;
		Optional<bool> darkened;
		Optional<bool> artwork;
		Optional<bool> back;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzCoverArtArchive fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzReleaseGroup {
		String id;
		size_t count;
		String title;
		String primaryType;
		String sortName;
		ArrayList<MusicBrainzArtist> artistCredit;
		std::shared_ptr<ArrayList<MusicBrainzRelease>> releases;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzReleaseGroup fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzRelease {
		String id;
		String title;
		String disambiguation;
		String asin;
		String statusId;
		String status;
		String date;
		String country;
		String quality;
		String barcode;
		ArrayList<MusicBrainzMedia> media;
		Optional<MusicBrainzCoverArtArchive> coverArtArchive;
		Optional<ArrayList<MusicBrainzArtistCredit>> artistCredit;
		Optional<ArrayList<MusicBrainzReleaseEvent>> releaseEvents;
		std::shared_ptr<MusicBrainzReleaseGroup> releaseGroup;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzRelease fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzReleaseEvent {
		Optional<MusicBrainzArea> area;
		Optional<String> date;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzReleaseEvent fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzRecording {
		String id;
		String title;
		String disambiguation;
		Optional<double> length;
		bool video;
		Optional<ArrayList<MusicBrainzRelease>> releases;
		Optional<ArrayList<MusicBrainzRelation>> relations;
		Optional<ArrayList<MusicBrainzArtistCredit>> artistCredit;
		Optional<ArrayList<MusicBrainzAlias>> aliases;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzRecording fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzTrack {
		String id;
		size_t number;
		String title;
		Optional<double> length;
		Optional<ArrayList<MusicBrainzArtistCredit>> artistCredit;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzTrack fromNapiObject(Napi::Object);
		#endif
	};

	struct MusicBrainzMedia {
		String title;
		size_t position;
		ArrayList<MusicBrainzTrack> tracks;
		size_t trackCount;
		size_t trackOffset;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzMedia fromNapiObject(Napi::Object);
		#endif
	};

	template<typename ItemType>
	struct MusicBrainzMatch: public ItemType {
		double score;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzMatch<ItemType> fromNapiObject(Napi::Object);
		#endif
	};

	template<typename ItemType>
	struct MusicBrainzSearchResult: public ItemType {
		Date created;
		size_t count;
		size_t offset;
		ArrayList<ItemType> items;
		
		#ifdef NODE_API_MODULE
		static MusicBrainzSearchResult<ItemType> fromNapiObject(Napi::Object, String type);
		#endif
	};
}

#include "MusicBrainzTypes.impl.h"
