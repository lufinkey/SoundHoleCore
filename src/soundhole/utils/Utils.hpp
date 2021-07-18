//
//  Utils.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/26/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh::utils {
	struct ExceptionDetails {
		String fullDescription;
		String message;
	};
	ExceptionDetails getExceptionDetails(std::exception_ptr error);

	String getTmpDirectoryPath();
	String getCacheDirectoryPath();


	template<typename Key,typename Value>
	inline const Value& getMapValueOrDefault(const std::map<Key,Value>& map, const Key& key, const Value& defaultValue) {
		auto it = map.find(key);
		if(it == map.end()) {
			return defaultValue;
		}
		return it->second;
	}


	void setPreferenceValue(String key, Json json);
	Json getPreferenceValue(String key);
}
