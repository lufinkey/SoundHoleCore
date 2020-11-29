//
//  BandcampMediaTypes.impl.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/22/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	#ifdef NODE_API_MODULE

	template<typename ItemType>
	Optional<BandcampFan::Section<ItemType>> BandcampFan::Section<ItemType>::maybeFromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined()) {
			return std::nullopt;
		}
		return fromNapiObject(obj);
	}

	template<typename ItemType>
	BandcampFan::Section<ItemType> BandcampFan::Section<ItemType>::fromNapiObject(Napi::Object obj) {
		return Section<ItemType>{
			.itemCount=(size_t)obj.Get("itemCount").As<Napi::Number>().Int64Value(),
			.batchSize=jsutils::optSizeFromNapiValue(obj.Get("batchSize")),
			.lastToken=jsutils::optStringFromNapiValue(obj.Get("lastToken")).valueOr(String()),
			.items=jsutils::arrayListFromNapiValue<ItemType>(obj.Get("items"), [](Napi::Value value) {
				return ItemType::fromNapiObject(value.As<Napi::Object>());
			})
		};
	}

	template<typename ItemType>
	BandcampFan::FollowItemNode<ItemType> BandcampFan::FollowItemNode<ItemType>::fromNapiObject(Napi::Object obj) {
		return FollowItemNode<ItemType>{
			.token=jsutils::nonNullStringPropFromNapiObject(obj,"token"),
			.dateFollowed=jsutils::nonNullStringPropFromNapiObject(obj,"dateFollowed"),
			.item=ItemType::fromNapiObject(obj.Get("item").As<Napi::Object>())
		};
	}

	template<typename ItemType>
	BandcampFanSectionPage<ItemType> BandcampFanSectionPage<ItemType>::fromNapiObject(Napi::Object obj) {
		return BandcampFanSectionPage<ItemType>{
			.hasMore=obj.Get("hasMore").As<Napi::Boolean>().Value(),
			.lastToken=jsutils::stringFromNapiValue(obj.Get("lastToken")),
			.items=jsutils::arrayListFromNapiValue<ItemType>(obj.Get("items"), [](Napi::Value value) {
				return ItemType::fromNapiObject(value.As<Napi::Object>());
			})
		};
	}

	#endif
}
