//
//  MusicBrainz.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/1/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "MusicBrainz.hpp"
#include "MusicBrainzError.hpp"
#include <soundhole/scripts/Scripts.hpp>
#include <soundhole/utils/js/JSUtils.hpp>
#include <soundhole/utils/js/JSWrapClass.impl.hpp>

namespace sh {
	MusicBrainz::MusicBrainz(Options options)
	: options(options) {
		// TODO load auth
	}

	MusicBrainz::~MusicBrainz() {
		//
	}


	String MusicBrainz::userAgent() const {
		String userAgent;
		userAgent.reserve(options.appName.size() + options.appVersion.size() + options.appContactInfo.size() + 6);
		if(!options.appName.empty()) {
			userAgent += options.appName;
		}
		if(!options.appVersion.empty()) {
			if(!userAgent.empty()) {
				userAgent += '/';
			}
			userAgent += options.appVersion;
		}
		if(!options.appContactInfo.empty()) {
			if(!userAgent.empty()) {
				userAgent += " ( ";
			}
			userAgent += options.appContactInfo;
			userAgent += " )";
		}
		return userAgent;
	}



	#pragma mark Template helper methods

	Promise<Json> MusicBrainz::sendRequest(utils::HttpMethod method, String endpoint, const std::map<String,String>& queryParams) {
		auto url = URL::parse("https://musicbrainz.org/ws/2/"+endpoint)
			.valueOrThrow(std::invalid_argument("invalid endpoint "+endpoint));
		auto queryItems = LinkedList<URL::QueryItem>();
		for(auto& pair : queryParams) {
			queryItems.pushBack(URL::QueryItem{
				.key = pair.first,
				.value = pair.second
			});
		}
		url.setQueryItems(queryItems);
		return utils::performHttpRequest(utils::HttpRequest{
			.url = url,
			.method = method,
			.headers = utils::HttpHeaders(Map<String,String>{
				{ "User-Agent", userAgent() }
			})
		}).map(nullptr, [](auto response) {
			if(response->statusCode < 200 || response->statusCode >= 300) {
				throw MusicBrainzError(MusicBrainzError::Code::REQUEST_FAILED, response->statusMessage);
			}
			std::string parseError;
			auto json = Json::parse(response->data.toString(), parseError);
			if(!parseError.empty()) {
				throw MusicBrainzError(MusicBrainzError::Code::BAD_DATA, "Failed to parse response: "+parseError);
			}
			return json;
		});
	}

	template<typename T>
	Promise<T> MusicBrainz::getEntity(String type, String mbid, ArrayList<String> include) {
		if(type.empty()) {
			return rejectWith(std::invalid_argument("type cannot be empty"));
		} else if(mbid.empty()) {
			return rejectWith(std::invalid_argument("mbid cannot be empty"));
		}
		auto queryObj = Map<String,String>();
		if(!include.empty()) {
			queryObj.put("inc", String::join(include, " "));
		}
		return sendRequest(utils::HttpMethod::GET, type+"/"+mbid, queryObj)
			.map(nullptr, [](auto json) {
				return T::fromJson(json);
			});
	}


	template<typename T>
	Promise<MusicBrainzSearchResult<T>> MusicBrainz::search(String type, String query, SearchOptions options) {
		if(query.empty()) {
			return rejectWith(std::invalid_argument("query cannot be empty"));
		}
		auto queryObj = Map<String,String>{
			{ "query", query }
		};
		if(!options.include.empty()) {
			queryObj.put("inc", String::join(options.include, " "));
		}
		if(options.offset.hasValue()) {
			queryObj.put("offset", std::to_string(options.offset.value()));
		}
		if(options.limit.hasValue()) {
			queryObj.put("limit", std::to_string(options.limit.value()));
		}
		return sendRequest(utils::HttpMethod::GET, type, queryObj)
			.map(nullptr, [=](auto json) {
				return MusicBrainzSearchResult<T>::fromJson(json, type);
			});
	}


	template<typename T>
	Promise<MusicBrainzSearchResult<T>> MusicBrainz::search(String type, QueryMap query, SearchOptions options) {
		String queryIncludes;
		auto queryObj = Map<String,String>();
		for(auto& pair : query) {
			if(pair.first == "inc" && pair.second.is<String>()) {
				queryIncludes = pair.second.get<String>();
			} else {
				if(pair.second.is<String>()) {
					queryObj.put(pair.first, pair.second.get<String>());
				} else if(pair.second.is<long>()) {
					queryObj.put(pair.first, std::to_string(pair.second.get<long>()));
				} else if(pair.second.is<double>()) {
					queryObj.put(pair.first, std::to_string(pair.second.get<double>()));
				} else {
					throw std::logic_error("invalid query value type");
				}
			}
		}
		if(!queryIncludes.empty() || !options.include.empty()) {
			auto includesStr = options.include.empty() ? queryIncludes : (String::join(options.include, " ") + " " + queryIncludes);
			queryObj.put("inc", includesStr);
		}
		if(options.offset.hasValue()) {
			queryObj.put("offset", std::to_string(options.offset.value()));
		}
		if(options.limit.hasValue()) {
			queryObj.put("limit", std::to_string(options.limit.value()));
		}
		return sendRequest(utils::HttpMethod::GET, type, queryObj)
			.map(nullptr, [=](const Json& json) {
				return MusicBrainzSearchResult<T>::fromJson(json, type);
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
