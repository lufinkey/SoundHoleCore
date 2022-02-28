//
//  MusicBrainzTypes.impl.h
//  SoundHoleCore
//
//  Created by Luis Finke on 2/6/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include <soundhole/common.hpp>
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	#ifdef NODE_API_MODULE
	template<typename ItemType>
	MusicBrainzMatch<ItemType> MusicBrainzMatch<ItemType>::fromNapiObject(Napi::Object obj) {
		return MusicBrainzMatch<ItemType>{
			ItemType::fromNapiObject(obj),
			.score = jsutils::optDoubleFromNapiValue(obj.Get("score"))
				.valueOrThrow(std::invalid_argument("missing 'score' property for MusicBrainzMatch"))
		};
	}
	#endif

	template<typename ItemType>
	MusicBrainzMatch<ItemType> MusicBrainzMatch<ItemType>::fromJson(const Json& json) {
		return MusicBrainzMatch<ItemType>{
			ItemType::fromJson(json),
			.score = jsutils::optDoubleFromJson(json["score"])
				.valueOrThrow(std::invalid_argument("missing 'score' property for MusicBrainzMatch"))
		};
	}

	#ifdef NODE_API_MODULE
	template<typename ItemType>
	MusicBrainzSearchResult<ItemType> MusicBrainzSearchResult<ItemType>::fromNapiObject(Napi::Object obj, std::string type) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined() || !obj.IsObject()) {
			throw std::invalid_argument("MusicBrainzSearchResult::fromNapiObject requires an object");
		}
		auto itemsKey = type + "s";
		return MusicBrainzSearchResult<ItemType>{
			.created = jsutils::optStringFromNapiValue(obj.Get("created")).map([](auto& str) {
				return Date::maybeFromISOString(str)
					.valueOrThrow(std::invalid_argument(str+" is not a valid date for MusicBrainzSearchResult.created"));
			}).valueOrThrow("missing property 'created' for MusicBrainzSearchResult"),
			.count = jsutils::nonNullSizePropFromNapiObject(obj, "count"),
			.offset = jsutils::nonNullSizePropFromNapiObject(obj, "offset"),
			.items = jsutils::arrayListFromNapiValue(obj.Get(itemsKey), [](Napi::Value itemObj) {
				return ItemType::fromNapiObject(itemObj.As<Napi::Object>());
			})
		};
	}
	#endif

	template<typename ItemType>
	MusicBrainzSearchResult<ItemType> MusicBrainzSearchResult<ItemType>::fromJson(const Json& json, std::string type) {
		if(json.is_null() || !json.is_object()) {
			throw std::invalid_argument("MusicBrainzSearchResult::fromJson requires an object");
		}
		auto itemsKey = type + "s";
		return MusicBrainzSearchResult<ItemType>{
			.created = jsutils::optStringFromJson(json["created"]).map([](auto& str) {
				return Date::maybeFromISOString(str)
					.valueOrThrow(std::invalid_argument(str+" is not a valid date for MusicBrainzSearchResult.created"));
			}).valueOrThrow("missing property 'created' for MusicBrainzSearchResult"),
			.count = jsutils::nonNullSizePropFromJson(json, "count"),
			.offset = jsutils::nonNullSizePropFromJson(json, "offset"),
			.items = jsutils::arrayListFromJson(json[itemsKey], [](const Json& itemJson) {
				return ItemType::fromJson(itemJson);
			})
		};
	}
}
