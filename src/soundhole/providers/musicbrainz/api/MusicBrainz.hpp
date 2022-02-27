//
//  MusicBrainz.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/1/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/js/JSWrapClass.hpp>
#include "MusicBrainzTypes.hpp"

namespace sh {
	class MusicBrainz: private JSWrapClass {
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
		
		struct SearchOptions {
			Optional<size_t> offset;
			Optional<size_t> limit;
			Optional<bool> dismax;
			ArrayList<String> include;
		};
		
		Promise<MusicBrainzArtist> getArtist(String mbid, ArrayList<String> include = {});
		Promise<MusicBrainzSearchResult<MusicBrainzArtist>> searchArtists(String query, SearchOptions options);
		Promise<MusicBrainzSearchResult<MusicBrainzArtist>> searchArtists(QueryMap query, SearchOptions options);
		Promise<MusicBrainzSearchResult<MusicBrainzArtist>> browseLinkedArtists(std::map<String,String> entities);
		
		Promise<MusicBrainzRelease> getRelease(String mbid, ArrayList<String> include = {});
		Promise<MusicBrainzSearchResult<MusicBrainzRelease>> searchReleases(String query, SearchOptions options);
		Promise<MusicBrainzSearchResult<MusicBrainzRelease>> searchReleases(QueryMap query, SearchOptions options);
		Promise<MusicBrainzSearchResult<MusicBrainzRelease>> browseLinkedReleases(std::map<String,String> entities);
		
		Promise<MusicBrainzReleaseGroup> getReleaseGroup(String mbid, ArrayList<String> include = {});
		Promise<MusicBrainzSearchResult<MusicBrainzReleaseGroup>> searchReleaseGroups(String query, SearchOptions options);
		Promise<MusicBrainzSearchResult<MusicBrainzReleaseGroup>> searchReleaseGroups(QueryMap query, SearchOptions options);
		Promise<MusicBrainzSearchResult<MusicBrainzReleaseGroup>> browseLinkedReleaseGroups(std::map<String,String> entities);
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		#ifdef NODE_API_MODULE
		template<typename Result>
		Promise<Result> performAsyncMusicBrainzFunc(String funcName, Function<std::vector<napi_value>(napi_env)> createArgs, Function<Result(napi_env,Napi::Value)> mapper);
		#endif
		
		template<typename T>
		Promise<T> getEntity(String type, String mbid, ArrayList<String> include);
		
		template<typename T>
		Promise<MusicBrainzSearchResult<T>> search(String type, String query, SearchOptions options);
		template<typename T>
		Promise<MusicBrainzSearchResult<T>> search(String type, QueryMap query, SearchOptions options);
		
		template<typename T>
		Promise<MusicBrainzSearchResult<T>> browseLinkedEntities(String type, std::map<String,String> linkage);
		
		napi_ref jsRef;
		Options options;
	};
}
