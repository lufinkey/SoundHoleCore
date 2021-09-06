//
//  LastFMSession.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFMSession.hpp"
#include "LastFMError.hpp"

namespace sh {
	LastFMSession LastFMSession::fromJson(const Json& json) {
		auto keyJson = json["key"];
		if(keyJson.string_value().empty()) {
			throw LastFMError(LastFMError::Code::BAD_DATA, "Missing required property 'key'");
		}
		auto subscriberJson = json["subscriber"];
		return LastFMSession{
			.name = json["name"].string_value(),
			.subscriber = subscriberJson.is_bool() ? subscriberJson.bool_value() : subscriberJson.is_number() ? (subscriberJson.number_value() != 0) : false,
			.key = keyJson.string_value()
		};
	}

	Json LastFMSession::toJson() const {
		return Json::object{
			{ "name", (std::string)name },
			{ "subscriber", subscriber },
			{ "key", (std::string)key }
		};
	}
}
