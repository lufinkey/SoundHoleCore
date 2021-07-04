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
	YoutubePage<T> YoutubePage<T>::fromJson(const Json& json) {
		return YoutubePage<T>{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.prevPageToken = json["prevPageToken"].string_value(),
			.nextPageToken = json["nextPageToken"].string_value(),
			.pageInfo = YoutubePageInfo::fromJson(json["pageInfo"]),
			.items = jsutils::arrayListFromJson(json["items"], [](auto& json) -> T {
				return T::fromJson(json);
			})
		};
	}

	template<typename T>
	template<typename Transform>
	auto YoutubePage<T>::map(Transform transform) const {
		using ReturnType = decltype(transform(std::declval<const T>()));
		return YoutubePage<ReturnType>{
			.kind=kind,
			.etag=etag,
			.prevPageToken=prevPageToken,
			.nextPageToken=nextPageToken,
			.pageInfo=YoutubePageInfo{
				.totalResults=pageInfo.totalResults,
				.resultsPerPage=pageInfo.resultsPerPage
			},
			.items=items.map(transform)
		};
	}



	template<typename T>
	YoutubeItemList<T> YoutubeItemList<T>::fromJson(const Json& json) {
		return YoutubeItemList<T>{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.items = jsutils::arrayListFromJson(json["items"], [](auto& json) -> T {
				return T::fromJson(json);
			})
		};
	}

	template<typename T>
	template<typename Transform>
	auto YoutubeItemList<T>::map(Transform transform) const {
		using ReturnType = decltype(transform(std::declval<T>()));
		return YoutubeItemList<ReturnType>{
			.kind=kind,
			.etag=etag,
			.items=items.map(transform)
		};
	}
}
