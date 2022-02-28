//
//  MusicBrainz.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/1/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/HttpClient.hpp>
#include <soundhole/utils/js/JSWrapClass.hpp>
#include "MusicBrainzTypes.hpp"

namespace sh {
	class MusicBrainz {
	public:
		using QueryItemValue = Variant<String,long,double>;
		using QueryMap = Map<String,QueryItemValue>;
		
		struct Options {
			String appName;
			String appVersion;
			String appContactInfo;
		};
		
		MusicBrainz(const MusicBrainz&) = delete;
		MusicBrainz& operator=(const MusicBrainz&) = delete;
		
		MusicBrainz(Options);
		~MusicBrainz();
		
		String userAgent() const;
		
		struct SearchOptions {
			Optional<size_t> offset;
			Optional<size_t> limit;
			ArrayList<String> include;
		};
		
		Promise<Json> sendRequest(utils::HttpMethod method, String endpoint, const std::map<String,String>& queryParams);
		
		Promise<MusicBrainzArtist> getArtist(String mbid, ArrayList<String> include = {});
		Promise<MusicBrainzSearchResult<MusicBrainzArtist>> searchArtists(String query, SearchOptions options);
		Promise<MusicBrainzSearchResult<MusicBrainzArtist>> searchArtists(QueryMap query, SearchOptions options);
		
		Promise<MusicBrainzRelease> getRelease(String mbid, ArrayList<String> include = {});
		Promise<MusicBrainzSearchResult<MusicBrainzRelease>> searchReleases(String query, SearchOptions options);
		Promise<MusicBrainzSearchResult<MusicBrainzRelease>> searchReleases(QueryMap query, SearchOptions options);
		
		Promise<MusicBrainzReleaseGroup> getReleaseGroup(String mbid, ArrayList<String> include = {});
		Promise<MusicBrainzSearchResult<MusicBrainzReleaseGroup>> searchReleaseGroups(String query, SearchOptions options);
		Promise<MusicBrainzSearchResult<MusicBrainzReleaseGroup>> searchReleaseGroups(QueryMap query, SearchOptions options);
		
	private:
		template<typename T>
		Promise<T> getEntity(String type, String mbid, ArrayList<String> include);
		
		template<typename T>
		Promise<MusicBrainzSearchResult<T>> search(String type, String query, SearchOptions options);
		template<typename T>
		Promise<MusicBrainzSearchResult<T>> search(String type, QueryMap query, SearchOptions options);
		
		Options options;
	};
}
