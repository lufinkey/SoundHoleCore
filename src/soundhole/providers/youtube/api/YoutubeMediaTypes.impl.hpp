//
//  YoutubeMediaTypes.impl.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/16/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	template<typename T>
	typename YoutubePage<T>::PageInfo YoutubePage<T>::PageInfo::fromJson(const Json& json) {
		return PageInfo{
			.totalResults = (size_t)json["totalResults"].number_value(),
			.resultsPerPage = (size_t)json["resultsPerPage"].number_value()
		};
	}
	
	template<typename T>
	YoutubePage<T> YoutubePage<T>::fromJson(const Json& json) {
		return YoutubePage<T>{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.prevPageToken = json["prevPageToken"].string_value(),
			.nextPageToken = json["nextPageToken"].string_value(),
			.regionCode = json["regionCode"].string_value(),
			.pageInfo = PageInfo::fromJson(json["pageInfo"]),
			.items = jsutils::arrayListFromJson<T>(json["items"], [](auto& json){
				return T::fromJson(json);
			})
		};
	}

	template<typename T>
	template<typename U>
	YoutubePage<U> YoutubePage<T>::map(Function<U(const T&)> mapper) const {
		return YoutubePage<U>{
			.kind=kind,
			.etag=etag,
			.prevPageToken=prevPageToken,
			.nextPageToken=nextPageToken,
			.regionCode=regionCode,
			.pageInfo=typename YoutubePage<U>::PageInfo{
				.totalResults=pageInfo.totalResults,
				.resultsPerPage=pageInfo.resultsPerPage
			},
			.items=items.template map<U>(mapper)
		};
	}
}
