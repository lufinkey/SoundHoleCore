//
//  LastFMAuth.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/6/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFMAuth.hpp"
#include "LastFMAPIRequest.hpp"
#include <soundhole/utils/SecureStore.hpp>

namespace sh {
	String LastFMAuth::APIConfig::webLoginBaseURL() const {
		auto urlParts = URL::Components(webLogin);
		urlParts.path = "/";
		return URL(urlParts).toString();
	}

	String LastFMAuth::APIConfig::webLoginHost() const {
		return URL(webLogin).host().valueOr(String());
	}

	String LastFMAuth::APIConfig::webLoginBaseHost() const {
		auto webLoginHost = this->webLoginHost();
		if(webLoginHost.startsWith("www.")) {
			webLoginHost = webLoginHost.substr(4);
		}
		return webLoginHost;
	}

	LastFMAuth::LastFMAuth(Options options, APIConfig apiConfig): _options(options), _apiConfig(apiConfig) {
		//
	}

	const LastFMAuth::Options& LastFMAuth::options() const {
		return _options;
	}

	const LastFMAuth::APIConfig& LastFMAuth::apiConfig() const {
		return _apiConfig;
	}

	const LastFMAPICredentials& LastFMAuth::apiCredentials() const {
		return _options.apiCredentials;
	}

	Optional<LastFMSession> LastFMAuth::session() const {
		return _session;
	}

	bool LastFMAuth::isLoggedIn() const {
		return _session.hasValue();
	}

	void LastFMAuth::load() {
		if(_options.sessionPersistKey.empty()) {
			return;
		}
		auto sessionData = SecureStore::getSecureData(_options.sessionPersistKey).toString();
		std::string parseError;
		auto sessionJson = Json::parse(sessionData, parseError);
		auto session = sessionJson.is_null() ? std::nullopt : maybeTry([&]() { return LastFMSession::fromJson(sessionJson); });
		this->_session = session;
	}

	void LastFMAuth::save() {
		if(_options.sessionPersistKey.empty()) {
			return;
		}
		if(_session) {
			SecureStore::setSecureData(_options.sessionPersistKey, Data(_session->toJson().dump()));
		} else {
			SecureStore::deleteSecureData(_options.sessionPersistKey);
		}
	}

	Promise<LastFMSession> LastFMAuth::getMobileSession(String username, String password) const {
		return LastFMAPIRequest{
			.apiRoot = _apiConfig.apiRoot,
			.apiMethod = "auth.getMobileSession",
			.httpMethod = utils::HttpMethod::POST,
			.params = {
				{ "username", username },
				{ "password", password }
			},
			.credentials = _options.apiCredentials,
			.session = std::nullopt
		}.perform().map(nullptr, [](Json response) {
			return LastFMSession::fromJson(response["session"]);
		});
	}

	Promise<Optional<Tuple<LastFMSession,LastFMWebAuthResult>>> LastFMAuth::authenticate() const {
		struct SharedData {
			Optional<LastFMSession> session;
		};
		auto sharedData = fgl::new$<SharedData>(SharedData{});
		return authenticateWeb([=](auto webResult) -> Promise<void> {
			if(webResult.username.empty()) {
				return rejectWith(std::runtime_error("Failed to get username parameter from web authenticator"));
			}
			if(webResult.password.empty()) {
				return rejectWith(std::runtime_error("Failed to get password parameter from web authenticator"));
			}
			return getMobileSession(webResult.username, webResult.password).map(nullptr, [=](auto session) {
				sharedData->session = session;
			});
		}).map(nullptr, [=](auto webResult) -> Optional<Tuple<LastFMSession,LastFMWebAuthResult>> {
			if(!webResult) {
				return std::nullopt;
			}
			return std::make_tuple(sharedData->session.value(), webResult.value());
		});
	}

	Promise<bool> LastFMAuth::login() {
		return authenticate().map([=](auto result) {
			if(!result) {
				return false;
			}
			auto [ session, webSession ] = result.value();
			this->applyWebAuthResult(webSession);
			this->_session = session;
			return true;
		});
	}

	void LastFMAuth::logout() {
		if(!_session) {
			return;
		}
		_session = std::nullopt;
		save();
	}
}
