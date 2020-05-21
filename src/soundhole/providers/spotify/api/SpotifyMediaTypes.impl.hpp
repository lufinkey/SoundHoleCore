//
//  SpotifyMediaTypes.impl.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	template<typename T>
	SpotifyPage<T> SpotifyPage<T>::fromJson(const Json& json) {
		auto previousString = json["previous"].string_value();
		auto nextString = json["next"].string_value();
		return SpotifyPage<T>{
			.href = json["href"].string_value(),
			.limit = (size_t)json["limit"].number_value(),
			.offset = (size_t)json["offset"].number_value(),
			.total = (size_t)json["total"].number_value(),
			.previous = (previousString != "null") ? nextString : "",
			.next = (nextString != "null") ? nextString : "",
			.items = jsutils::arrayListFromJson<T>(json["items"], [](auto& item) -> T {
				return T::fromJson(item);
			})
		};
	}

	template<typename T>
	Optional<SpotifyPage<T>> SpotifyPage<T>::maybeFromJson(const Json& json) {
		if(json.is_null()) {
			return std::nullopt;
		}
		return fromJson(json);
	}

	template<typename T>
	template<typename U>
	SpotifyPage<U> SpotifyPage<T>::map(Function<U(const T&)> mapper) const {
		return SpotifyPage<U>{
			.href=href,
			.limit=limit,
			.offset=offset,
			.total=total,
			.previous=previous,
			.next=next,
			.items=items.template map<U>(mapper)
		};
	}
}
