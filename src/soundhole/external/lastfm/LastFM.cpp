//
//  LastFM.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFM.hpp"
#include "LastFMError.hpp"
#include "LastFMAPIRequest.hpp"
#include <soundhole/utils/SecureStore.hpp>

namespace sh {
	LastFM::LastFM(Options options): auth(new LastFMAuth(options.auth)) {
		auth->load();
	}

	LastFM::~LastFM() {
		delete auth;
	}

	Promise<Json> LastFM::sendRequest(utils::HttpMethod httpMethod, String apiMethod, std::map<String,String> params, bool usesAuth) {
		return LastFMAPIRequest{
			.apiMethod = apiMethod,
			.httpMethod = httpMethod,
			.params = params,
			.credentials = auth->apiCredentials(),
			.session = usesAuth ? auth->session() : std::nullopt
		}.perform();
	}

	Promise<bool> LastFM::login() {
		return auth->login();
	}

	void LastFM::logout() {
		auth->logout();
	}

	bool LastFM::isLoggedIn() const {
		return auth->isLoggedIn();
	}


	Promise<LastFMScrobbleResponse> LastFM::scrobble(LastFMScrobbleRequest request) {
		return sendRequest(utils::HttpMethod::POST, "track.scrobble", request.toQueryItems()).map(nullptr, [](Json response) {
			return LastFMScrobbleResponse::fromJson(response);
		});
	}

	Promise<void> LastFM::loveTrack(String track, String artist) {
		return sendRequest(utils::HttpMethod::POST, "track.love", {
			{ "track", track },
			{ "artist", artist }
		}).toVoid();
	}
}
