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



	Promise<LastFMArtistSearchResults> LastFM::searchArtist(SearchArtistOptions options) {
		auto params = Map<String,String>{
			{ "artist", options.artist }
		};
		if(options.page.hasValue()) {
			params["page"] = std::to_string(options.page.value());
		}
		if(options.limit.hasValue()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		return sendRequest(utils::HttpMethod::GET, "artist.search", params).map(nullptr, [](Json json) {
			return LastFMArtistSearchResults::fromJson(json["results"]);
		});
	}

	Promise<LastFMArtistInfo> LastFM::getArtistInfo(GetArtistInfoOptions options) {
		auto params = Map<String,String>{
			{ "artist", options.artist }
		};
		if(!options.mbid.empty()) {
			params["mbid"] = options.mbid;
		}
		if(!options.username.empty()) {
			params["username"] = options.username;
		}
		if(!options.lang.empty()) {
			params["lang"] = options.lang;
		}
		if(options.autocorrect.hasValue()) {
			params["autocorrect"] = options.autocorrect.value() ? "1" : "0";
		}
		return sendRequest(utils::HttpMethod::GET, "artist.getInfo", params).map(nullptr, [](Json json) {
			return LastFMArtistInfo::fromJson(json["artist"]);
		});
	}

	Promise<LastFMArtistItemsPage<LastFMArtistTopAlbum>> LastFM::getArtistTopAlbums(GetArtistTopItemsOptions options) {
		auto params = Map<String,String>{
			{ "artist", options.artist }
		};
		if(!options.mbid.empty()) {
			params["mbid"] = options.mbid;
		}
		if(options.autocorrect.hasValue()) {
			params["autocorrect"] = options.autocorrect.value() ? "1" : "0";
		}
		if(options.page) {
			params["page"] = std::to_string(options.page.value());
		}
		if(options.limit) {
			params["limit"] = std::to_string(options.limit.value());
		}
		return sendRequest(utils::HttpMethod::GET, "artist.getTopAlbums", params)
			.map(nullptr, [](Json json) {
				json = json["topalbums"];
				return LastFMArtistItemsPage<LastFMArtistTopAlbum>{
					.items = jsutils::singleOrArrayListFromJson(json["album"], [](auto& itemJson) {
						return LastFMArtistTopAlbum::fromJson(itemJson);
					}),
					.attrs = LastFMArtistItemsPageAttrs::fromJson(json["@attr"])
				};
			});
	}

	Promise<LastFMArtistItemsPage<LastFMTrackInfo>> LastFM::getArtistTopTracks(GetArtistTopItemsOptions options) {
		auto params = Map<String,String>{
			{ "artist", options.artist }
		};
		if(!options.mbid.empty()) {
			params["mbid"] = options.mbid;
		}
		if(options.autocorrect.hasValue()) {
			params["autocorrect"] = options.autocorrect.value() ? "1" : "0";
		}
		if(options.page) {
			params["page"] = std::to_string(options.page.value());
		}
		if(options.limit) {
			params["limit"] = std::to_string(options.limit.value());
		}
		return sendRequest(utils::HttpMethod::GET, "artist.getTopTracks", params)
			.map(nullptr, [](Json json) {
				json = json["toptracks"];
				return LastFMArtistItemsPage<LastFMTrackInfo>{
					.items = jsutils::singleOrArrayListFromJson(json["track"], [](auto& itemJson) {
						return LastFMTrackInfo::fromJson(itemJson);
					}),
					.attrs = LastFMArtistItemsPageAttrs::fromJson(json["@attr"])
				};
			});
	}



	Promise<LastFMTrackSearchResults> LastFM::searchTrack(SearchTrackOptions options) {
		auto params = Map<String,String>{
			{ "track", options.track }
		};
		if(!options.artist.empty()) {
			params["artist"] = options.artist;
		}
		if(options.page.hasValue()) {
			params["page"] = std::to_string(options.page.value());
		}
		if(options.limit.hasValue()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		return sendRequest(utils::HttpMethod::GET, "track.search", params).map(nullptr, [](Json json) {
			return LastFMTrackSearchResults::fromJson(json["results"]);
		});
	}

	Promise<LastFMTrackInfo> LastFM::getTrackInfo(GetTrackInfoOptions options) {
		auto params = Map<String,String>{
			{ "track", options.track },
			{ "artist", options.artist }
		};
		if(!options.username.empty()) {
			params["username"] = options.username;
		}
		if(options.autocorrect.hasValue()) {
			params["autocorrect"] = options.autocorrect.value() ? "1" : "0";
		}
		return sendRequest(utils::HttpMethod::GET, "track.getInfo", params).map(nullptr, [](Json json) {
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



	Promise<LastFMAlbumSearchResults> LastFM::searchAlbum(SearchAlbumOptions options) {
		auto params = Map<String,String>{
			{ "album", options.album }
		};
		if(!options.artist.empty()) {
			params["artist"] = options.artist;
		}
		if(options.page.hasValue()) {
			params["page"] = std::to_string(options.page.value());
		}
		if(options.limit.hasValue()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		return sendRequest(utils::HttpMethod::GET, "album.search", params).map(nullptr, [](Json json) {
			return LastFMAlbumSearchResults::fromJson(json["results"]);
		});
	}

	Promise<LastFMAlbumInfo> LastFM::getAlbumInfo(GetAlbumInfoOptions options) {
		auto params = Map<String,String>{
			{ "album", options.album }
		};
		if(!options.artist.empty()) {
			params["artist"] = options.artist;
		}
		if(!options.mbid.empty()) {
			params["mbid"] = options.mbid;
		}
		if(!options.username.empty()) {
			params["username"] = options.username;
		}
		if(!options.lang.empty()) {
			params["lang"] = options.lang;
		}
		if(options.autocorrect.hasValue()) {
			params["autocorrect"] = options.autocorrect.value() ? "1" : "0";
		}
		return sendRequest(utils::HttpMethod::GET, "album.getInfo", params).map(nullptr, [](Json json) {
			return LastFMAlbumInfo::fromJson(json["album"]);
		});
	}

	



    Promise<LastFMUserInfo> LastFM::getUserInfo(String user) {
		auto params = Map<String,String>{
			{ "user", user }
		};
        return sendRequest(utils::HttpMethod::GET, "user.getInfo", params).map([](Json json) {
            return LastFMUserInfo::fromJson(json["user"]);
        });
    }

	Promise<LastFMUserInfo> LastFM::getCurrentUserInfo() {
		return sendRequest(utils::HttpMethod::GET, "user.getInfo", {}).map(nullptr, [](Json json) {
			return LastFMUserInfo::fromJson(json["user"]);
		});
	}

	Promise<LastFMUserItemsPage<LastFMUserRecentTrack>> LastFM::getUserRecentTracks(String user, GetRecentTracksOptions options) {
		auto params = Map<String,String>{
			{ "user", user }
		};
		if(options.from.hasValue()) {
			params["from"] = std::to_string((long long)options.from->secondsSince1970());
		}
		if(options.to.hasValue()) {
			params["to"] = std::to_string((long long)options.to->secondsSince1970());
		}
		if(options.page.hasValue()) {
			params["page"] = std::to_string(options.page.value());
		}
		if(options.limit.hasValue()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.extended.hasValue()) {
			params["extended"] = options.extended.value() ? "1" : "0";
		}
		return sendRequest(utils::HttpMethod::GET, "user.getRecentTrack", params).map(nullptr, [](Json json) {
			json = json["recenttracks"];
			return LastFMUserItemsPage<LastFMUserRecentTrack>{
				.items = jsutils::singleOrArrayListFromJson(json["track"], [](auto& itemJson) {
					return LastFMUserRecentTrack::fromJson(itemJson);
				}),
				.attrs = LastFMUserItemsPageAttrs::fromJson(json["@attr"])
			};
		});
	}

	Promise<LastFMUserItemsPage<LastFMUserRecentTrack>> LastFM::getCurrentUserRecentTracks(GetRecentTracksOptions options) {
		auto session = auth->session();
		if(!session.hasValue()) {
			return rejectWith(LastFMError(LastFMError::Code::NOT_LOGGED_IN, "User is not logged in"));
		}
		auto params = Map<String,String>{
			{ "user", session->name }
		};
		if(options.from.hasValue()) {
			params["from"] = std::to_string((long long)options.from->secondsSince1970());
		}
		if(options.to.hasValue()) {
			params["to"] = std::to_string((long long)options.to->secondsSince1970());
		}
		if(options.page.hasValue()) {
			params["page"] = std::to_string(options.page.value());
		}
		if(options.limit.hasValue()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.extended.hasValue()) {
			params["extended"] = options.extended.value() ? "1" : "0";
		}
		return sendRequest(utils::HttpMethod::GET, "user.getRecentTrack", params).map(nullptr, [](Json json) {
			json = json["recenttracks"];
			return LastFMUserItemsPage<LastFMUserRecentTrack>{
				.items = jsutils::singleOrArrayListFromJson(json["track"], [](auto& itemJson) {
					return LastFMUserRecentTrack::fromJson(itemJson);
				}),
				.attrs = LastFMUserItemsPageAttrs::fromJson(json["@attr"])
			};
		});
	}
}
