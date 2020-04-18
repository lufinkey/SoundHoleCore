//
//  Spotify.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/28/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Spotify.hpp"
#include <soundhole/utils/HttpClient.hpp>

namespace sh {
	Spotify::Spotify(Options options)
	: playerOptions(options.player),
	auth(new SpotifyAuth(options.auth)), player(nullptr) {
		auth->load();
	}
	
	Spotify::~Spotify() {
		if(player != nullptr) {
			player->setAuth(nullptr);
			player->logout();
			player->stop();
			player = nullptr;
		}
		delete auth;
	}

	SpotifyAuth* Spotify::getAuth() {
		return auth;
	}

	const SpotifyAuth* Spotify::getAuth() const {
		return auth;
	}

	SpotifyPlayer* Spotify::getPlayer() {
		return player;
	}

	const SpotifyPlayer* Spotify::getPlayer() const {
		return player;
	}
	
	Promise<bool> Spotify::login(LoginOptions options) {
		return auth->login(options);
	}
	
	Promise<void> Spotify::logout() {
		auto player = this->player;
		auth->logout();
		if(player != nullptr) {
			return player->logout();
		}
		return Promise<void>::resolve();
	}
	
	bool Spotify::isLoggedIn() const {
		return auth->isLoggedIn();
	}
	
	
	
	Promise<void> Spotify::startPlayer() {
		if(!auth->isLoggedIn()) {
			return Promise<void>::reject(
				SpotifyError(SpotifyError::Code::NOT_LOGGED_IN, "Not logged in")
			);
		} else if(!auth->hasStreamingScope()) {
			return Promise<void>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "Missing spotify streaming scope")
			);
		}
		std::unique_lock<std::mutex> lock(playerMutex);
		if(this->player != nullptr) {
			auto player = this->player;
			if(player->isStarted()) {
				return Promise<void>::resolve();
			} else {
				lock.unlock();
				return player->start();
			}
		} else {
			auto player = SpotifyPlayer::shared();
			if(player->getAuth() != nullptr && player->getAuth() != this->auth) {
				return Promise<void>::reject(
					SpotifyError(SpotifyError::Code::PLAYER_IN_USE, "Player is being used by another SpotifyAuth instance")
				);
			}
			player->setAuth(this->auth);
			player->setOptions(playerOptions);
			this->player = player;
			for(auto listener : playerListeners) {
				this->player->addEventListener(listener);
			}
			if(player->isStarted()) {
				return Promise<void>::resolve();
			}
			return player->start();
		}
	}
	
	void Spotify::stopPlayer() {
		std::unique_lock<std::mutex> lock(playerMutex);
		if(player != nullptr) {
			player->setAuth(nullptr);
			player->setOptions(SpotifyPlayer::Options());
			player->logout();
			player->stop();
			for(auto listener : playerListeners) {
				player->removeEventListener(listener);
			}
			player = nullptr;
		}
	}



	void Spotify::addPlayerEventListener(SpotifyPlayerEventListener* listener) {
		playerListeners.pushBack(listener);
		if(player != nullptr) {
			player->addEventListener(listener);
		}
	}

	void Spotify::removePlayerEventListener(SpotifyPlayerEventListener* listener) {
		playerListeners.removeLastEqual(listener);
		if(player != nullptr) {
			player->removeEventListener(listener);
		}
	}
	
	
	
	Promise<void> Spotify::playURI(String uri, PlayOptions options) {
		return prepareForPlayer().then([=]() -> Promise<void> {
			return player->playURI(uri, options);
		});
	}
	
	Promise<void> Spotify::queueURI(String uri) {
		return prepareForPlayer().then([=]() -> Promise<void> {
			return player->queueURI(uri);
		});
	}
	
	Promise<void> Spotify::skipToNext() {
		return prepareForPlayer().then([=]() -> Promise<void> {
			return player->skipToNext();
		});
	}
	
	Promise<void> Spotify::skipToPrevious() {
		return prepareForPlayer().then([=]() -> Promise<void> {
			return player->skipToPrevious();
		});
	}
	
	Promise<void> Spotify::seek(double position) {
		return prepareForPlayer().then([=]() -> Promise<void> {
			return player->seek(position);
		});
	}
	
	Promise<void> Spotify::setPlaying(bool playing) {
		return prepareForPlayer().then([=]() -> Promise<void> {
			return player->setPlaying(playing);
		});
	}
	
	Promise<void> Spotify::setShuffling(bool shuffling) {
		return prepareForPlayer().then([=]() -> Promise<void> {
			return player->setShuffling(shuffling);
		});
	}
	
	Promise<void> Spotify::setRepeating(bool repeating) {
		return prepareForPlayer().then([=]() -> Promise<void> {
			return player->setRepeating(repeating);
		});
	}
	
	SpotifyPlayer::State Spotify::getPlaybackState() const {
		if(player != nullptr) {
			return player->getState();
		}
		return SpotifyPlayer::State{
			.playing = false,
			.repeating = false,
			.shuffling = false,
			.activeDevice = false,
			.position = 0
		};
	}
	
	SpotifyPlayer::Metadata Spotify::getPlaybackMetadata() const {
		if(player != nullptr) {
			return player->getMetadata();
		}
		return SpotifyPlayer::Metadata{
			.previousTrack = std::nullopt,
			.currentTrack = std::nullopt,
			.nextTrack = std::nullopt
		};
	}
	
	Promise<void> Spotify::prepareForPlayer() {
		if(player != nullptr && player->isStarted()) {
			return Promise<void>::resolve();
		}
		return startPlayer();
	}
	
	
	
	Promise<void> Spotify::prepareForRequest() {
		return auth->renewSessionIfNeeded().then([=](bool renewed){
			// ignore
		}, [=](std::exception_ptr error) {
			// ignore
		});
	}
	
	Promise<Json> Spotify::sendRequest(utils::HttpMethod method, String endpoint, Json params) {
		return prepareForRequest().then([=]() -> Promise<Json> {
			bool usesQueryParams = ArrayList<utils::HttpMethod>{
				utils::HttpMethod::GET,
				utils::HttpMethod::HEAD,
				utils::HttpMethod::DELETE
			}.contains(method);
			String url = "https://api.spotify.com/" + endpoint;
			if(usesQueryParams && params.object_items().size() > 0) {
				std::map<String,String> queryParams;
				for(auto& pair : params.object_items()) {
					queryParams[pair.first] = pair.second.string_value();
				}
				url += "?" + utils::makeQueryString(queryParams);
			}
			std::map<String,String> headers;
			if(!usesQueryParams && !params.is_null()) {
				headers["Content-Type"] = "application/json; charset=utf-8";
			}
			auto session = auth->getSession();
			if(session) {
				headers["Authorization"] = "Bearer "+session->getAccessToken();
			}
			auto request = utils::HttpRequest{
				.url = Url(url),
				.method = method,
				.headers = headers,
				.data = (!usesQueryParams) ? params.dump() : ""
			};
			return utils::performHttpRequest(request)
			.then([=](utils::SharedHttpResponse response) -> Promise<utils::SharedHttpResponse> {
				// renew and retry request if response is 401
				if(response->statusCode == 401) {
					return auth->renewSession().then([=](bool renewed) -> Promise<utils::SharedHttpResponse> {
						if(!renewed) {
							return Promise<utils::SharedHttpResponse>::resolve(response);
						}
						return utils::performHttpRequest(request);
					}).except([=](std::exception_ptr errorPtr) -> utils::SharedHttpResponse {
						try {
							std::rethrow_exception(errorPtr);
						} catch(SpotifyError& error) {
							if(error.getCode() == SpotifyError::Code::NOT_LOGGED_IN) {
								throw error;
							}
						} catch(...) {
							// ignore
						}
						return response;
					});
				}
				return Promise<utils::SharedHttpResponse>::resolve(response);
			})
			.map<Json>([=](utils::SharedHttpResponse response) -> Json {
				Json result;
				auto contentTypeIt = response->headers.find("content-type");
				String contentType = (contentTypeIt != response->headers.end()) ? contentTypeIt->second : "";
				if(!contentType.empty()) {
					contentType = contentType.split(';').front().trim().toLowerCase();
				}
				if(!contentType.empty() && contentType == "application/json" && response->statusCode != 204) {
					std::string parseError;
					result = Json::parse((std::string)response->data, parseError);
					if(!parseError.empty()) {
						throw SpotifyError(SpotifyError::Code::BAD_RESPONSE, "Failed to parse response json: "+parseError);

					}
					auto errorObj = result["error"];
					if(!errorObj.is_null()) {
						auto errorDescObj = result["error_description"];
						if(!errorDescObj.is_null()) {
							throw SpotifyError(SpotifyError::Code::REQUEST_FAILED, errorDescObj.string_value(), {
								{ "spotifyCode", String(errorObj.string_value()) }
							});
						}
						else if(errorObj.is_object()) {
							String errorMessage = errorObj["message"].string_value();
							if(errorMessage.empty()) {
								errorMessage = "Request failed";
							}
							std::map<String,Any> details = {
								{ "statusCode", response->statusCode }
							};
							if(response->statusCode == 429) {
								auto retryAfterIt = response->headers.find("retry-after");
								String retryAfter = (retryAfterIt != response->headers.end()) ? retryAfterIt->second : "";
								if(!retryAfter.empty()) {
									errorMessage += ". Retry after "+retryAfter+" seconds";
								}
								details["retryAfter"] = retryAfter.toArithmeticValue<int>();
							}
							throw SpotifyError(SpotifyError::Code::REQUEST_FAILED, errorMessage, details);
						} else {
							throw SpotifyError(SpotifyError::Code::BAD_RESPONSE, "Unknown error response format");
						}
					}
				}
				return result;
			});
		});
	}
	
	
	
	Promise<SpotifyUser> Spotify::getMe() {
		return sendRequest(utils::HttpMethod::GET, "v1/me").map<SpotifyUser>([](auto json) {
			return SpotifyUser::fromJson(json);
		});
	}
	
	Promise<SpotifySearchResults> Spotify::search(String query, SearchOptions options) {
		std::map<std::string,Json> params;
		if(options.types.empty()) {
			params["type"] = (std::string)String::join(ArrayList<String>{ "track", "artist", "album", "playlist" }, ",");
		} else {
			params["type"] = (std::string)String::join(options.types, ",");
		}
		if(!options.market.empty()) {
			params["market"] = (std::string)options.market;
		}
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		params["q"] = (std::string)query;
		return sendRequest(utils::HttpMethod::GET, "v1/search", params).map<SpotifySearchResults>([](auto json) {
			return SpotifySearchResults::fromJson(json);
		});
	}
	
	Promise<SpotifyAlbum> Spotify::getAlbum(String albumId, GetAlbumOptions options) {
		if(albumId.empty()) {
			return Promise<SpotifyAlbum>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "albumId cannot be empty")
			);
		}
		std::map<std::string,Json> params;
		if(!options.market.empty()) {
			params["market"] = (std::string)options.market;
		}
		return sendRequest(utils::HttpMethod::GET, "v1/albums/"+albumId, params).map<SpotifyAlbum>([](auto json) {
			return SpotifyAlbum::fromJson(json);
		});
	}
	
	Promise<ArrayList<SpotifyAlbum>> Spotify::getAlbums(ArrayList<String> albumIds, GetAlbumsOptions options) {
		std::map<std::string,Json> params;
		if(!options.market.empty()) {
			params["market"] = (std::string)options.market;
		}
		params["ids"] = (std::string)String::join(albumIds, ",");
		return sendRequest(utils::HttpMethod::GET, "v1/albums", params).map<ArrayList<SpotifyAlbum>>([](auto json) {
			auto albumItems = json["albums"].array_items();
			ArrayList<SpotifyAlbum> albums;
			albums.reserve(albumItems.size());
			for(auto& albumJson : albumItems) {
				albums.pushBack(SpotifyAlbum::fromJson(albumJson));
			}
			return albums;
		});
	}
	
	Promise<SpotifyPage<SpotifyTrack>> Spotify::getAlbumTracks(String albumId, GetAlbumTracksOptions options) {
		if(albumId.empty()) {
			return Promise<SpotifyPage<SpotifyTrack>>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "albumId cannot be empty")
			);
		}
		std::map<std::string,Json> params;
		if(!options.market.empty()) {
			params["market"] = (std::string)options.market;
		}
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/albums/"+albumId+"/tracks", params).map<SpotifyPage<SpotifyTrack>>([](auto json) {
			return SpotifyPage<SpotifyTrack>::fromJson(json);
		});
	}
	
	Promise<SpotifyArtist> Spotify::getArtist(String artistId) {
		if(artistId.empty()) {
			return Promise<SpotifyArtist>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "artistId cannot be empty")
			);
		}
		return sendRequest(utils::HttpMethod::GET, "v1/artists/"+artistId).map<SpotifyArtist>([](auto json) {
			return SpotifyArtist::fromJson(json);
		});
	}
	
	Promise<ArrayList<SpotifyArtist>> Spotify::getArtists(ArrayList<String> artistIds) {
		std::map<std::string,Json> params;
		params["ids"] = (std::string)String::join(artistIds, ",");
		return sendRequest(utils::HttpMethod::GET, "v1/artists", params).map<ArrayList<SpotifyArtist>>([](auto json) {
			auto artistItems = json["artists"].array_items();
			ArrayList<SpotifyArtist> artists;
			artists.reserve(artistItems.size());
			for(auto& artistJson : artistItems) {
				artists.pushBack(SpotifyArtist::fromJson(artistJson));
			}
			return artists;
		});
	}
	
	Promise<SpotifyPage<SpotifyAlbum>> Spotify::getArtistAlbums(String artistId, GetArtistAlbumsOptions options) {
		if(artistId.empty()) {
			return Promise<SpotifyPage<SpotifyAlbum>>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "artistId cannot be empty")
			);
		}
		std::map<std::string,Json> params;
		if(!options.market.empty()) {
			params["market"] = (std::string)options.market;
		}
		if(!options.includeGroups.empty()) {
			params["include_groups"] = (std::string)String::join(options.includeGroups, ",");
		}
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/artists/"+artistId+"/albums", params).map<SpotifyPage<SpotifyAlbum>>([](auto json) {
			return SpotifyPage<SpotifyAlbum>::fromJson(json);
		});
	}
	
	Promise<ArrayList<SpotifyTrack>> Spotify::getArtistTopTracks(String artistId, String country, GetArtistTopTracksOptions options) {
		if(artistId.empty()) {
			return Promise<ArrayList<SpotifyTrack>>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "artistId cannot be empty")
			);
		} else if(country.empty()) {
			return Promise<ArrayList<SpotifyTrack>>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "country cannot be empty")
			);
		}
		std::map<std::string,Json> params;
		if(!options.market.empty()) {
			params["market"] = (std::string)options.market;
		}
		params["country"] = (std::string)country;
		return sendRequest(utils::HttpMethod::GET, "v1/artists/"+artistId+"/top-tracks", params).map<ArrayList<SpotifyTrack>>([](auto json) {
			auto trackItems = json["tracks"].array_items();
			ArrayList<SpotifyTrack> tracks;
			tracks.reserve(trackItems.size());
			for(auto& trackJson : trackItems) {
				tracks.pushBack(SpotifyTrack::fromJson(trackJson));
			}
			return tracks;
		});
	}
	
	Promise<ArrayList<SpotifyArtist>> Spotify::getArtistRelatedArtists(String artistId) {
		if(artistId.empty()) {
			return Promise<ArrayList<SpotifyArtist>>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "artistId cannot be empty")
			);
		}
		return sendRequest(utils::HttpMethod::GET, "v1/artists/"+artistId+"/related-artists").map<ArrayList<SpotifyArtist>>([](auto json) {
			auto artistItems = json["artists"].array_items();
			ArrayList<SpotifyArtist> artists;
			artists.reserve(artistItems.size());
			for(auto& artistJson : artistItems) {
				artists.pushBack(SpotifyArtist::fromJson(artistJson));
			}
			return artists;
		});
	}
	
	Promise<SpotifyTrack> Spotify::getTrack(String trackId, GetTrackOptions options) {
		if(trackId.empty()) {
			return Promise<SpotifyTrack>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "trackId cannot be empty")
			);
		}
		std::map<std::string,Json> params;
		if(!options.market.empty()) {
			params["market"] = (std::string)options.market;
		}
		return sendRequest(utils::HttpMethod::GET, "v1/tracks/"+trackId, params).map<SpotifyTrack>([](auto json) {
			return SpotifyTrack::fromJson(json);
		});
	}
	
	Promise<ArrayList<SpotifyTrack>> Spotify::getTracks(ArrayList<String> trackIds, GetTracksOptions options) {
		std::map<std::string,Json> params;
		if(!options.market.empty()) {
			params["market"] = (std::string)options.market;
		}
		params["ids"] = (std::string)String::join(trackIds, ",");
		return sendRequest(utils::HttpMethod::GET, "v1/tracks", params).map<ArrayList<SpotifyTrack>>([](auto json) {
			auto trackItems = json["tracks"].array_items();
			ArrayList<SpotifyTrack> tracks;
			tracks.reserve(trackItems.size());
			for(auto& trackJson : trackItems) {
				tracks.pushBack(SpotifyTrack::fromJson(trackJson));
			}
			return tracks;
		});
	}
	
	Promise<Json> Spotify::getTrackAudioAnalysis(String trackId) {
		if(trackId.empty()) {
			return Promise<Json>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "trackId cannot be empty")
			);
		}
		return sendRequest(utils::HttpMethod::GET, "v1/audio-analysis/"+trackId);
	}
	
	Promise<Json> Spotify::getTrackAudioFeatures(String trackId) {
		if(trackId.empty()) {
			return Promise<Json>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "trackId cannot be empty")
			);
		}
		return sendRequest(utils::HttpMethod::GET, "v1/audio-features/"+trackId);
	}
	
	Promise<Json> Spotify::getTracksAudioFeatures(ArrayList<String> trackIds) {
		std::map<std::string,Json> params;
		params["ids"] = (std::string)String::join(trackIds, ",");
		return sendRequest(utils::HttpMethod::GET, "v1/audio-features", params);
	}
	
	Promise<SpotifyPlaylist> Spotify::getPlaylist(String playlistId, GetPlaylistOptions options) {
		if(playlistId.empty()) {
			return Promise<SpotifyPlaylist>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "playlistId cannot be empty")
			);
		}
		std::map<std::string,Json> params;
		if(!options.market.empty()) {
			params["market"] = (std::string)options.market;
		}
		if(!options.fields.empty()) {
			params["fields"] = (std::string)options.fields;
		}
		return sendRequest(utils::HttpMethod::GET, "v1/playlists/"+playlistId, params).map<SpotifyPlaylist>([](auto json) {
			return SpotifyPlaylist::fromJson(json);
		});
	}
	
	Promise<SpotifyPage<SpotifyPlaylist::Item>> Spotify::getPlaylistTracks(String playlistId, GetPlaylistTracksOptions options) {
		if(playlistId.empty()) {
			return Promise<SpotifyPage<SpotifyPlaylist::Item>>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "playlistId cannot be empty")
			);
		}
		std::map<std::string,Json> params;
		if(!options.market.empty()) {
			params["market"] = (std::string)options.market;
		}
		if(!options.fields.empty()) {
			params["fields"] = (std::string)options.fields;
		}
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/playlists/"+playlistId+"/tracks", params).map<SpotifyPage<SpotifyPlaylist::Item>>([](auto json) {
			return SpotifyPage<SpotifyPlaylist::Item>::fromJson(json);
		});
	}
}
