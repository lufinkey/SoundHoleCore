//
//  BandcampSession.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampSession.hpp"

namespace sh {
	Optional<BandcampSession> BandcampSession::fromJson(const Json& json) {
		if(json.is_null()) {
			return std::nullopt;
		}
		auto clientId = json["clientId"].string_value();
		auto identity = json["identity"].string_value();
		if(clientId.empty() || identity.empty()) {
			return std::nullopt;
		}
		auto cookiesJson = json["cookies"].array_items();
		ArrayList<String> cookies;
		cookies.reserve(cookiesJson.size());
		for(auto& cookieJson : cookiesJson) {
			if(cookieJson.is_string()) {
				cookies.pushBack(cookieJson.string_value());
			}
		}
		return BandcampSession(clientId, identity, cookies);
	}

	BandcampSession::BandcampSession(String clientId, String identity, ArrayList<String> cookies)
	: clientId(clientId), identity(identity), cookies(cookies) {
		//
	}

	const String& BandcampSession::getClientId() const {
		return clientId;
	}

	const String& BandcampSession::getIdentity() const {
		return identity;
	}

	const ArrayList<String>& BandcampSession::getCookies() const {
		return cookies;
	}

	bool BandcampSession::equals(const BandcampSession& cmp) const {
		if(clientId != cmp.clientId || identity != cmp.identity || cookies.size() != cmp.cookies.size()) {
			return false;
		}
		for(size_t i=0; i<cookies.size(); i++) {
			auto& a = cookies[i];
			auto& b = cmp.cookies[i];
			if(a != b) {
				return false;
			}
		}
		return true;
	}



	Json BandcampSession::toJson() const {
		return Json::object{
			{ "clientId", (std::string)clientId },
			{ "identity", (std::string)identity },
			{ "cookies", cookies.map([=](const String& cookie) -> Json {
				return Json((std::string)cookie);
			}) }
		};
	}
}
