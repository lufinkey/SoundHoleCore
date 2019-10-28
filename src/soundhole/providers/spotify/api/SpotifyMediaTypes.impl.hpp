//
//  SpotifyMediaTypes.impl.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/js/JSWrapClass.hpp>

namespace sh {
	template<typename T>
	SpotifyPage<T> SpotifyPage<T>::fromJson(const Json& json) {
		return SpotifyPage<T>{
			.href = json["href"].string_value(),
			.limit = (size_t)json["limit"].number_value(),
			.offset = (size_t)json["offset"].number_value(),
			.total = (size_t)json["total"].number_value(),
			.previous = json["previous"].string_value(),
			.next = json["next"].string_value(),
			.items = JSWrapClass::arrayListFromJson<T>(json["items"], [](auto& item) -> T {
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
}
