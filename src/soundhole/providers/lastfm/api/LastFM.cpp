//
//  LastFM.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFM.hpp"
#include "LastFMAPIRequest.hpp"
#include <soundhole/utils/js/JSUtils.hpp>
#include <soundhole/utils/SecureStore.hpp>

namespace sh {
	const LastFM::APIConfig LastFM::APICONFIG_LASTFM = LastFM::APIConfig{
		.apiRoot = "https://ws.audioscrobbler.com/2.0",
		.webLogin = "https://www.last.fm/login",
		.webSessionCookieName = "sessionid",
		.checkSessionCookieLoggedIn = [](String sessionCookie) -> bool {
			return sessionCookie.startsWith(".");
		}
	};

	LastFM::LastFM(Options options, APIConfig apiConfig): auth(new LastFMAuth(options.auth, apiConfig)) {
		auth->load();
	}

	LastFM::~LastFM() {
		delete auth;
	}

	Promise<Json> LastFM::sendRequest(utils::HttpMethod httpMethod, String apiMethod, Map<String,String> params, bool usesAuth, bool includeSignature) {
		return LastFMAPIRequest{
			.apiRoot = auth->apiConfig().apiRoot,
			.apiMethod = apiMethod,
			.httpMethod = httpMethod,
			.params = params,
			.credentials = auth->apiCredentials(),
			.session = usesAuth ? auth->session() : std::nullopt,
            .includeSignature = includeSignature
		}.perform();
	}

	LastFMAuth* LastFM::getAuth() {
		return auth;
	}

	const LastFMAuth* LastFM::getAuth() const {
		return auth;
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



	Promise<LastFMArtistSearchResults> LastFM::searchArtist(LastFMArtistSearchRequest request) {
		auto params = Map<String,String>{
			{ "artist", request.artist }
		};
		if(request.page.hasValue()) {
			params["page"] = std::to_string(request.page.value());
		}
		if(request.limit.hasValue()) {
			params["limit"] = std::to_string(request.limit.value());
		}
		return sendRequest(utils::HttpMethod::GET, "artist.search", params).map([](Json json) {
			return LastFMArtistSearchResults::fromJson(json["results"]);
		});
	}

	Promise<LastFMArtistInfo> LastFM::getArtistInfo(LastFMArtistInfoRequest request) {
		auto params = Map<String,String>{
			{ "artist", request.artist }
		};
		if(!request.mbid.empty()) {
			params["mbid"] = request.mbid;
		}
		if(!request.username.empty()) {
			params["username"] = request.username;
		}
		if(!request.lang.empty()) {
			params["lang"] = request.lang;
		}
		if(request.autocorrect.hasValue()) {
			params["autocorrect"] = request.autocorrect.value() ? "1" : "0";
		}
		return sendRequest(utils::HttpMethod::GET, "artist.getInfo", params).map([](Json json) {
			return LastFMArtistInfo::fromJson(json["artist"]);
		});
	}

	Promise<LastFMArtistTopItemsPage<LastFMArtistTopAlbum>> LastFM::getArtistTopAlbums(LastFMArtistTopItemsRequest request) {
		return sendRequest(utils::HttpMethod::GET, "artist.getTopAlbums", request.toQueryParams())
			.map([](Json json) {
				json = json["topalbums"];
				return LastFMArtistTopItemsPage<LastFMArtistTopAlbum>{
					.items = jsutils::singleOrArrayListFromJson(json["album"], [](auto& itemJson) {
						return LastFMArtistTopAlbum::fromJson(itemJson);
					}),
					.attrs = LastFMArtistTopItemsPageAttrs::fromJson(json["@attr"])
				};
			});
	}

	Promise<LastFMArtistTopItemsPage<LastFMTrackInfo>> LastFM::getArtistTopTracks(LastFMArtistTopItemsRequest request) {
		return sendRequest(utils::HttpMethod::GET, "artist.getTopTracks", request.toQueryParams())
			.map([](Json json) {
				json = json["toptracks"];
				return LastFMArtistTopItemsPage<LastFMTrackInfo>{
					.items = jsutils::singleOrArrayListFromJson(json["track"], [](auto& itemJson) {
						return LastFMTrackInfo::fromJson(itemJson);
					}),
					.attrs = LastFMArtistTopItemsPageAttrs::fromJson(json["@attr"])
				};
			});
	}



	Promise<LastFMTrackSearchResults> LastFM::searchTrack(LastFMTrackSearchRequest request) {
		auto params = Map<String,String>{
			{ "track", request.track }
		};
		if(!request.artist.empty()) {
			params["artist"] = request.artist;
		}
		if(request.page.hasValue()) {
			params["page"] = std::to_string(request.page.value());
		}
		if(request.limit.hasValue()) {
			params["limit"] = std::to_string(request.limit.value());
		}
		return sendRequest(utils::HttpMethod::GET, "track.search", params).map([](Json json) {
			return LastFMTrackSearchResults::fromJson(json["results"]);
		});
	}

	Promise<LastFMTrackInfo> LastFM::getTrackInfo(LastFMTrackInfoRequest request) {
		auto params = Map<String,String>{
			{ "track", request.track },
			{ "artist", request.artist }
		};
		if(!request.username.empty()) {
			params["username"] = request.username;
		}
		if(request.autocorrect.hasValue()) {
			params["autocorrect"] = request.autocorrect.value() ? "1" : "0";
		}
		return sendRequest(utils::HttpMethod::GET, "track.getInfo", params).map([](Json json) {
			return LastFMTrackInfo::fromJson(json["track"]);
		});
	}

	Promise<void> LastFM::loveTrack(String track, String artist) {
		return sendRequest(utils::HttpMethod::POST, "track.love", {
			{ "track", track },
			{ "artist", artist }
		}).toVoid();
	}

    Promise<void> LastFM::unloveTrack(String track, String artist) {
        return sendRequest(utils::HttpMethod::POST, "track.unlove", {
            { "track", track },
            { "artist", artist }
        }).toVoid();
    }



	Promise<LastFMAlbumSearchResults> LastFM::searchAlbum(LastFMAlbumSearchRequest request) {
		auto params = Map<String,String>{
			{ "album", request.album }
		};
		if(!request.artist.empty()) {
			params["artist"] = request.artist;
		}
		if(request.page.hasValue()) {
			params["page"] = std::to_string(request.page.value());
		}
		if(request.limit.hasValue()) {
			params["limit"] = std::to_string(request.limit.value());
		}
		return sendRequest(utils::HttpMethod::GET, "album.search", params).map([](Json json) {
			return LastFMAlbumSearchResults::fromJson(json["results"]);
		});
	}

	Promise<LastFMAlbumInfo> LastFM::getAlbumInfo(LastFMAlbumInfoRequest request) {
		auto params = Map<String,String>{
			{ "album", request.album }
		};
		if(!request.artist.empty()) {
			params["artist"] = request.artist;
		}
		if(!request.mbid.empty()) {
			params["mbid"] = request.mbid;
		}
		if(!request.username.empty()) {
			params["username"] = request.username;
		}
		if(!request.lang.empty()) {
			params["lang"] = request.lang;
		}
		if(request.autocorrect.hasValue()) {
			params["autocorrect"] = request.autocorrect.value() ? "1" : "0";
		}
		return sendRequest(utils::HttpMethod::GET, "album.getInfo", params).map([](Json json) {
			return LastFMAlbumInfo::fromJson(json["album"]);
		});
	}

	



    Promise<LastFMUserInfo> LastFM::getUserInfo(String user) {
        auto params = Map<String,String>();
        if(!user.empty()) {
            params["user"] = user;
        }
        return sendRequest(utils::HttpMethod::GET, "user.getInfo", params).map([](Json json) {
            return LastFMUserInfo::fromJson(json["user"]);
        });
    }
}
