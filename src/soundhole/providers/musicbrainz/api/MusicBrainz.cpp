//
//  MusicBrainz.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/1/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "MusicBrainz.hpp"
#include <soundhole/scripts/Scripts.hpp>
#include <soundhole/utils/js/JSUtils.hpp>
#include <soundhole/utils/js/JSWrapClass.impl.hpp>

namespace sh {
	MusicBrainz::MusicBrainz(Options options)
	: jsRef(nullptr), options(options) {
		// TODO load auth
	}

	MusicBrainz::~MusicBrainz() {
		auto jsRef = this->jsRef;
		if(jsRef != nullptr) {
			queueJSDestruct([=](napi_env env) {
				auto napiRef = Napi::ObjectReference(env, jsRef);
				if(napiRef.Unref() == 0) {
					napiRef.Reset();
				} else {
					napiRef.SuppressDestruct();
				}
			});
		}
		// TODO delete auth object
	}

	void MusicBrainz::initializeJS(napi_env env) {
		if(this->jsRef == nullptr) {
			// create options
			auto mbOptions = Napi::Object::New(env);
			if(!options.appName.empty()) {
				mbOptions.Set("appName", (std::string)options.appName);
			}
			if(!options.appVersion.empty()) {
				mbOptions.Set("appVersion", (std::string)options.appVersion);
			}
			if(!options.appContactInfo.empty()) {
				mbOptions.Set("appContactInfo", (std::string)options.appContactInfo);
			}
			
			// instantiate bandcamp
			auto jsExports = scripts::getJSExports(env);
			auto MusicBrainzApiClass = jsExports.Get("MusicBrainzApi").As<Napi::Function>();
			auto musicbrainz = MusicBrainzApiClass.New({ mbOptions });
			auto musicbrainzRef = Napi::ObjectReference::New(musicbrainz, 1);
			musicbrainzRef.SuppressDestruct();
			jsRef = musicbrainzRef;
		}
	}



	#pragma mark Template helper methods

	template<typename Result>
	Promise<Result> MusicBrainz::performAsyncMusicBrainzFunc(String funcName, Function<std::vector<napi_value>(napi_env)> createArgs, Function<Result(napi_env,Napi::Value)> mapper) {
		return performAsyncFunc<Result>([=](napi_env env) {
			return jsutils::jsValue<Napi::Object>(env, this->jsRef);
		}, funcName, createArgs, mapper, {
			.beforeFuncCall = nullptr,
			.afterFuncFinish = nullptr
		});
	}

	template<typename T>
	Promise<T> MusicBrainz::getEntity(String type, String mbid, ArrayList<String> include) {
		return performAsyncMusicBrainzFunc<T>("getEntity", [=](napi_env env) {
			auto args = std::vector<napi_value>{
				Napi::String::New(env, (std::string)type),
				Napi::String::New(env, (std::string)mbid),
			};
			if(!include.empty()) {
				auto includeArr = Napi::Array::New(env, include.size());
				for(uint32_t i=0; i<include.size(); i++) {
					includeArr.Set(i, Napi::String::New(env, include[i]));
				}
				args.push_back(includeArr);
			}
			return args;
		}, [](napi_env env, Napi::Value value) {
			return T::fromNapiObject(value.As<Napi::Object>());
		});
	}


	template<typename T>
	Promise<MusicBrainzSearchResult<T>> MusicBrainz::search(String type, String query, SearchOptions options) {
		return performAsyncMusicBrainzFunc<MusicBrainzSearchResult<T>>("search", [=](napi_env env) {
			auto queryObj = Napi::Object::New(env);
			queryObj.Set("query", query);
			if(!options.include.empty()) {
				auto includeArr = Napi::Array::New(env, options.include.size());
				for(uint32_t i=0; i<options.include.size(); i++) {
					includeArr.Set(i, Napi::String::New(env, options.include[i]));
				}
				queryObj.Set("inc", includeArr);
			}
			if(options.offset.hasValue()) {
				queryObj.Set("offset", Napi::Number::New(env, (double)options.offset.value()));
			}
			if(options.limit.hasValue()) {
				queryObj.Set("limit", Napi::Number::New(env, (double)options.limit.value()));
			}
			return std::vector<napi_value>{
				Napi::String::New(env, (std::string)type),
				queryObj
			};
		}, [=](napi_env env, Napi::Value value) {
			return MusicBrainzSearchResult<T>::fromNapiObject(value.As<Napi::Object>(), type);
		});
	}


	template<typename T>
	Promise<MusicBrainzSearchResult<T>> MusicBrainz::search(String type, QueryMap query, SearchOptions options) {
		return performAsyncMusicBrainzFunc<MusicBrainzSearchResult<T>>("search", [=](napi_env env) {
			ArrayList<String> queryIncludes;
			auto queryObj = Napi::Object::New(env);
			for(auto& pair : query) {
				if(pair.first == "inc" && pair.second.is<String>()) {
					// append query includes
					queryIncludes = pair.second.get<String>().split(' ');
				} else {
					// append query field
					if(pair.second.is<String>()) {
						queryObj.Set((std::string)pair.first, Napi::String::New(env, (std::string)pair.second.get<String>()));
					}
					else if(pair.second.is<long>()) {
						queryObj.Set((std::string)pair.first, Napi::Number::New(env, (double)pair.second.get<long>()));
					}
					else if(pair.second.is<double>()) {
						queryObj.Set((std::string)pair.first, Napi::Number::New(env, (double)pair.second.get<double>()));
					}
					else {
						throw std::logic_error("invalid query value type");
					}
				}
			}
			if(!options.include.empty() || !queryIncludes.empty()) {
				auto includes = options.include;
				for(auto& inc : queryIncludes) {
					if(!inc.empty() && !includes.contains(inc)) {
						includes.push_back(inc);
					}
				}
				auto includeArr = Napi::Array::New(env, includes.size());
				for(uint32_t i=0; i<includes.size(); i++) {
					includeArr.Set(i, Napi::String::New(env, includes[i]));
				}
				queryObj.Set("inc", includeArr);
			}
			if(options.offset.hasValue()) {
				queryObj.Set("offset", Napi::Number::New(env, (double)options.offset.value()));
			}
			if(options.limit.hasValue()) {
				queryObj.Set("limit", Napi::Number::New(env, (double)options.limit.value()));
			}
			return std::vector<napi_value>{
				Napi::String::New(env, (std::string)type),
				queryObj
			};
		}, [=](napi_env env, Napi::Value value) {
			return MusicBrainzSearchResult<T>::fromNapiObject(value.As<Napi::Object>(), type);
		});
	}



	#pragma mark Artist

	Promise<MusicBrainzArtist> MusicBrainz::getArtist(String mbid, ArrayList<String> include) {
		return getEntity<MusicBrainzArtist>("artist", mbid, include);
	}
	Promise<MusicBrainzSearchResult<MusicBrainzArtist>> MusicBrainz::searchArtists(String query, SearchOptions options) {
		return search<MusicBrainzArtist>("artist", query, options);
	}
	Promise<MusicBrainzSearchResult<MusicBrainzArtist>> MusicBrainz::searchArtists(QueryMap query, SearchOptions options) {
		return search<MusicBrainzArtist>("artist", query, options);
	}


	#pragma mark Release

	Promise<MusicBrainzRelease> MusicBrainz::getRelease(String mbid, ArrayList<String> include) {
		return getEntity<MusicBrainzRelease>("release", mbid, include);
	}
	Promise<MusicBrainzSearchResult<MusicBrainzRelease>> MusicBrainz::searchReleases(String query, SearchOptions options) {
		return search<MusicBrainzRelease>("release", query, options);
	}
	Promise<MusicBrainzSearchResult<MusicBrainzRelease>> MusicBrainz::searchReleases(QueryMap query, SearchOptions options) {
		return search<MusicBrainzRelease>("release", query, options);
	}


	#pragma mark Release Group

	Promise<MusicBrainzReleaseGroup> MusicBrainz::getReleaseGroup(String mbid, ArrayList<String> include) {
		return getEntity<MusicBrainzReleaseGroup>("release-group", mbid, include);
	}
	Promise<MusicBrainzSearchResult<MusicBrainzReleaseGroup>> MusicBrainz::searchReleaseGroups(String query, SearchOptions options) {
		return search<MusicBrainzReleaseGroup>("release-group", query, options);
	}
	Promise<MusicBrainzSearchResult<MusicBrainzReleaseGroup>> MusicBrainz::searchReleaseGroups(QueryMap query, SearchOptions options) {
		return search<MusicBrainzReleaseGroup>("release-group", query, options);
	}
}
