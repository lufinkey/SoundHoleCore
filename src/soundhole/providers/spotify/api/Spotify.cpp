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

	#pragma mark login
	
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
	
	

	#pragma mark player
	
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
	
	

	#pragma mark metadata
	
	Promise<void> Spotify::prepareForRequest() {
		return auth->renewSessionIfNeeded().then([=](bool renewed){
			// ignore
		}, [=](std::exception_ptr error) {
			// ignore
		});
	}
	
	Promise<Json> Spotify::sendRequest(utils::HttpMethod method, String endpoint, std::map<String,String> queryParams, Json bodyParams) {
		return prepareForRequest().then([=]() -> Promise<Json> {
			String url = "https://api.spotify.com/" + endpoint;
			if(queryParams.size() > 0) {
				url += "?" + utils::makeQueryString(queryParams);
			}
			std::map<String,String> headers;
			if(!bodyParams.is_null()) {
				headers["Content-Type"] = "application/json; charset=utf-8";
			}
			auto session = auth->getSession();
			if(session) {
				headers["Authorization"] = session->getTokenType()+" "+session->getAccessToken();
			}
			auto request = utils::HttpRequest{
				.url = Url(url),
				.method = method,
				.headers = headers,
				.data = (!bodyParams.is_null()) ? bodyParams.dump() : ""
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
			.map([=](utils::SharedHttpResponse response) -> Json {
				Json result;
				auto contentTypeIt = response->headers.find("Content-Type");
				String contentType = (contentTypeIt != response->headers.end()) ? contentTypeIt->second : "";
				if(!contentType.empty()) {
					contentType = contentType.split(';').front().trim().toLowerCase();
				}
				if(!contentType.empty() && contentType == "application/json" && response->statusCode != 204) {
					std::string parseError;
					result = Json::parse((std::string)response->data, parseError);
					if(!parseError.empty()) {
						throw SpotifyError(SpotifyError::Code::BAD_DATA, "Failed to parse response json: "+parseError);
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
								auto retryAfterIt = response->headers.find("Retry-After");
								String retryAfter = (retryAfterIt != response->headers.end()) ? retryAfterIt->second : "";
								if(!retryAfter.empty()) {
									errorMessage += ". Retry after "+retryAfter+" seconds";
								}
								details["retryAfter"] = retryAfter.toArithmeticValue<double>();
							}
							throw SpotifyError(SpotifyError::Code::REQUEST_FAILED, errorMessage, details);
						} else {
							throw SpotifyError(SpotifyError::Code::BAD_DATA, "Unknown error response format");
						}
					}
				}
				return result;
			});
		});
	}



	#pragma mark metadata: my library

	Promise<SpotifyUser> Spotify::getMe() {
		return sendRequest(utils::HttpMethod::GET, "v1/me").map([](auto json) -> SpotifyUser {
			return SpotifyUser::fromJson(json);
		});
	}

	Promise<SpotifyPage<SpotifySavedTrack>> Spotify::getMyTracks(GetMyTracksOptions options) {
		std::map<String,String> params;
		if(!options.market.empty()) {
			params["market"] = options.market;
		}
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/me/tracks", params).map([](auto json) -> SpotifyPage<SpotifySavedTrack> {
			return SpotifyPage<SpotifySavedTrack>::fromJson(json);
		});
	}

	Promise<SpotifyPage<SpotifySavedAlbum>> Spotify::getMyAlbums(GetMyAlbumsOptions options) {
		std::map<String,String> params;
		if(!options.market.empty()) {
			params["market"] = options.market;
		}
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/me/albums", params).map([](auto json) -> SpotifyPage<SpotifySavedAlbum> {
			return SpotifyPage<SpotifySavedAlbum>::fromJson(json);
		});
	}

	Promise<SpotifyPage<SpotifyPlaylist>> Spotify::getMyPlaylists(GetMyPlaylistsOptions options) {
		std::map<String,String> params;
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/me/playlists", params).map([](auto json) -> SpotifyPage<SpotifyPlaylist> {
			return SpotifyPage<SpotifyPlaylist>::fromJson(json);
		});
	}


	
	#pragma mark metadata: search

	Promise<SpotifySearchResults> Spotify::search(String query, SearchOptions options) {
		std::map<String,String> params;
		if(options.types.empty()) {
			params["type"] = String::join({ "track", "artist", "album", "playlist" }, ",");
		} else {
			params["type"] = String::join(options.types, ",");
		}
		if(!options.market.empty()) {
			params["market"] = options.market;
		}
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		params["q"] = query;
		return sendRequest(utils::HttpMethod::GET, "v1/search", params).map([](auto json) -> SpotifySearchResults {
			return SpotifySearchResults::fromJson(json);
		});
	}

	#pragma mark metadata: albums
	
	Promise<SpotifyAlbum> Spotify::getAlbum(String albumId, GetAlbumOptions options) {
		if(albumId.empty()) {
			return Promise<SpotifyAlbum>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "albumId cannot be empty")
			);
		}
		std::map<String,String> params;
		if(!options.market.empty()) {
			params["market"] = options.market;
		}
		return sendRequest(utils::HttpMethod::GET, "v1/albums/"+albumId, params).map([](auto json) -> SpotifyAlbum {
			return SpotifyAlbum::fromJson(json);
		});
	}
	
	Promise<ArrayList<SpotifyAlbum>> Spotify::getAlbums(ArrayList<String> albumIds, GetAlbumsOptions options) {
		std::map<String,String> params;
		if(!options.market.empty()) {
			params["market"] = options.market;
		}
		params["ids"] = String::join(albumIds, ",");
		return sendRequest(utils::HttpMethod::GET, "v1/albums", params).map([](auto json) -> ArrayList<SpotifyAlbum> {
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
		std::map<String,String> params;
		if(!options.market.empty()) {
			params["market"] = options.market;
		}
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/albums/"+albumId+"/tracks", params).map([](auto json) -> SpotifyPage<SpotifyTrack> {
			return SpotifyPage<SpotifyTrack>::fromJson(json);
		});
	}



	#pragma mark metadata: artists

	Promise<SpotifyArtist> Spotify::getArtist(String artistId) {
		if(artistId.empty()) {
			return Promise<SpotifyArtist>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "artistId cannot be empty")
			);
		}
		return sendRequest(utils::HttpMethod::GET, "v1/artists/"+artistId).map([](auto json) -> SpotifyArtist {
			return SpotifyArtist::fromJson(json);
		});
	}
	
	Promise<ArrayList<SpotifyArtist>> Spotify::getArtists(ArrayList<String> artistIds) {
		std::map<String,String> params;
		params["ids"] = String::join(artistIds, ",");
		return sendRequest(utils::HttpMethod::GET, "v1/artists", params).map([](auto json) -> ArrayList<SpotifyArtist> {
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
		std::map<String,String> params;
		if(!options.country.empty()) {
			params["country"] = options.country;
		}
		if(!options.includeGroups.empty()) {
			params["include_groups"] = String::join(options.includeGroups, ",");
		}
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/artists/"+artistId+"/albums", params).map([](auto json) -> SpotifyPage<SpotifyAlbum> {
			return SpotifyPage<SpotifyAlbum>::fromJson(json);
		});
	}
	
	Promise<ArrayList<SpotifyTrack>> Spotify::getArtistTopTracks(String artistId, String country) {
		if(artistId.empty()) {
			return Promise<ArrayList<SpotifyTrack>>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "artistId cannot be empty")
			);
		} else if(country.empty()) {
			return Promise<ArrayList<SpotifyTrack>>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "country cannot be empty")
			);
		}
		std::map<String,String> params = {
			{"country", country}
		};
		return sendRequest(utils::HttpMethod::GET, "v1/artists/"+artistId+"/top-tracks", params).map([](auto json) -> ArrayList<SpotifyTrack> {
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
		return sendRequest(utils::HttpMethod::GET, "v1/artists/"+artistId+"/related-artists").map([](auto json) -> ArrayList<SpotifyArtist> {
			auto artistItems = json["artists"].array_items();
			ArrayList<SpotifyArtist> artists;
			artists.reserve(artistItems.size());
			for(auto& artistJson : artistItems) {
				artists.pushBack(SpotifyArtist::fromJson(artistJson));
			}
			return artists;
		});
	}



	#pragma mark metadata: tracks

	Promise<SpotifyTrack> Spotify::getTrack(String trackId, GetTrackOptions options) {
		if(trackId.empty()) {
			return Promise<SpotifyTrack>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "trackId cannot be empty")
			);
		}
		std::map<String,String> params;
		if(!options.market.empty()) {
			params["market"] = options.market;
		}
		return sendRequest(utils::HttpMethod::GET, "v1/tracks/"+trackId, params).map([](auto json) -> SpotifyTrack {
			return SpotifyTrack::fromJson(json);
		});
	}
	
	Promise<ArrayList<SpotifyTrack>> Spotify::getTracks(ArrayList<String> trackIds, GetTracksOptions options) {
		std::map<String,String> params;
		if(!options.market.empty()) {
			params["market"] = options.market;
		}
		params["ids"] = String::join(trackIds, ",");
		return sendRequest(utils::HttpMethod::GET, "v1/tracks", params).map([](auto json) -> ArrayList<SpotifyTrack> {
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
		std::map<String,String> params;
		params["ids"] = String::join(trackIds, ",");
		return sendRequest(utils::HttpMethod::GET, "v1/audio-features", params);
	}



	#pragma mark metadata: playlists
	
	Promise<SpotifyPlaylist> Spotify::getPlaylist(String playlistId, GetPlaylistOptions options) {
		if(playlistId.empty()) {
			return Promise<SpotifyPlaylist>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "playlistId cannot be empty"));
		}
		std::map<String,String> params;
		if(!options.market.empty()) {
			params["market"] = options.market;
		}
		if(!options.fields.empty()) {
			params["fields"] = options.fields;
		}
		return sendRequest(utils::HttpMethod::GET, "v1/playlists/"+playlistId, params).map([](auto json) -> SpotifyPlaylist {
			return SpotifyPlaylist::fromJson(json);
		});
	}

	Promise<SpotifyPlaylist> Spotify::createPlaylist(String name, CreatePlaylistOptions options) {
		std::map<std::string,Json> params = {
			{ "name", (std::string)name }
		};
		if(!options.description.empty()) {
			params["description"] = (std::string)options.description;
		}
		if(options.isPublic) {
			params["public"] = options.isPublic.value();
		}
		if(options.isCollaborative) {
			params["collaborative"] = options.isCollaborative.value();
		}
		return sendRequest(utils::HttpMethod::POST, "v1/me/playlists", {}, params).map([](auto json) -> SpotifyPlaylist {
			return SpotifyPlaylist::fromJson(json);
		});
	}

	Promise<SpotifyPlaylist> Spotify::createUserPlaylist(String userId, String name, CreatePlaylistOptions options) {
		std::map<std::string,Json> params = {
			{ "name", (std::string)name }
		};
		if(!options.description.empty()) {
			params["description"] = (std::string)options.description;
		}
		if(options.isPublic) {
			params["public"] = options.isPublic.value();
		}
		if(options.isCollaborative) {
			params["collaborative"] = options.isCollaborative.value();
		}
		return sendRequest(utils::HttpMethod::POST, "v1/users/"+userId+"/playlists", {}, params).map([](auto json) -> SpotifyPlaylist {
			return SpotifyPlaylist::fromJson(json);
		});
	}

	Promise<void> Spotify::updatePlaylist(String playlistId, UpdatePlaylistOptions options) {
		if(playlistId.empty()) {
			return Promise<void>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "playlistId cannot be empty"));
		}
		std::map<std::string,Json> params;
		if(options.name) {
			params["name"] = (std::string)options.name.value();
		}
		if(options.description) {
			params["description"] = (std::string)options.description.value();
		}
		if(options.isPublic) {
			params["public"] = options.isPublic.value();
		}
		if(options.isCollaborative) {
			params["collaborative"] = options.isCollaborative.value();
		}
		return sendRequest(utils::HttpMethod::PUT, "v1/playlists/"+playlistId, {}, params).toVoid();
	}



	#pragma mark metadata: playlist tracks

	Promise<SpotifyPage<SpotifyPlaylist::Item>> Spotify::getPlaylistTracks(String playlistId, GetPlaylistTracksOptions options) {
		if(playlistId.empty()) {
			return Promise<SpotifyPage<SpotifyPlaylist::Item>>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "playlistId cannot be empty"));
		}
		std::map<String,String> params;
		if(!options.market.empty()) {
			params["market"] = options.market;
		}
		if(!options.fields.empty()) {
			params["fields"] = options.fields;
		}
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/playlists/"+playlistId+"/tracks", params).map([](auto json) -> SpotifyPage<SpotifyPlaylist::Item> {
			return SpotifyPage<SpotifyPlaylist::Item>::fromJson(json);
		});
	}

	Promise<SpotifyPlaylist::AddResult> Spotify::addPlaylistTracks(String playlistId, ArrayList<String> trackURIs, AddPlaylistTracksOptions options) {
		if(playlistId.empty()) {
			return Promise<SpotifyPlaylist::AddResult>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "playlistId cannot be empty"));
		}
		if(trackURIs.empty()) {
			return Promise<SpotifyPlaylist::AddResult>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "trackURIs cannot be empty"));
		}
		auto params = Json::object{
			{ "uris", trackURIs.map([](auto& uri) -> Json {
				return Json(uri);
			}) }
		};
		if(options.position) {
			params["position"] = Json((int)options.position.value());
		}
		return sendRequest(utils::HttpMethod::POST, "v1/playlists/"+playlistId+"/tracks", {}, params).map([](auto json) -> SpotifyPlaylist::AddResult {
			return SpotifyPlaylist::AddResult::fromJson(json);
		});
	}

	Promise<SpotifyPlaylist::MoveResult> Spotify::movePlaylistTracks(String playlistId, size_t rangeStart, size_t insertBefore, MovePlaylistTracksOptions options) {
		if(playlistId.empty()) {
			return Promise<SpotifyPlaylist::MoveResult>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "playlistId cannot be empty")
			);
		}
		auto params = Json::object{
			{ "range_start", (int)rangeStart },
			{ "insert_before", (int)insertBefore }
		};
		if(options.count) {
			params["range_length"] = (int)options.count.value();
		}
		if(!options.snapshotId.empty()) {
			params["snapshot_id"] = (std::string)options.snapshotId;
		}
		return sendRequest(utils::HttpMethod::PUT, "v1/playlists/"+playlistId+"/tracks", {}, params).map([](auto json) -> SpotifyPlaylist::MoveResult {
			return SpotifyPlaylist::MoveResult::fromJson(json);
		});
	}

	Promise<SpotifyPlaylist::RemoveResult> Spotify::removePlaylistTracks(String playlistId, ArrayList<PlaylistTrackMarker> tracks, RemovePlaylistTracksOptions options) {
		if(tracks.empty()) {
			return Promise<SpotifyPlaylist::RemoveResult>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "tracks cannot be empty")
			);
		}
		auto params = Json::object{
			{ "tracks", tracks.map([](auto& track) -> Json {
				auto json = Json::object{
					{ "uri", Json(track.uri) }
				};
				if(track.positions.size() > 0) {
					json["positions"] = track.positions.map([](size_t position) -> Json {
						return Json((int)position);
					});
				}
				return Json(json);
			}) }
		};
		if(!options.snapshotId.empty()) {
			params["snapshot_id"] = (std::string)options.snapshotId;
		}
		return sendRequest(utils::HttpMethod::DELETE, "v1/playlists/"+playlistId+"/tracks", {}, params).map([](auto json) -> SpotifyPlaylist::RemoveResult {
			return SpotifyPlaylist::RemoveResult::fromJson(json);
		});
	}



	#pragma mark metadata: users

	Promise<SpotifyUser> Spotify::getUser(String userId) {
		if(userId.empty()) {
			return Promise<SpotifyUser>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "userId cannot be empty")
			);
		}
		return sendRequest(utils::HttpMethod::GET, "v1/users/"+userId).map([](auto json) -> SpotifyUser {
			return SpotifyUser::fromJson(json);
		});
	}

	struct GetUserPlaylistsOptions {
		Optional<size_t> limit;
		Optional<size_t> offset;
	};
	Promise<SpotifyPage<SpotifyPlaylist>> Spotify::getUserPlaylists(String userId, GetUserPlaylistsOptions options) {
		if(userId.empty()) {
			return Promise<SpotifyPage<SpotifyPlaylist>>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "userId cannot be empty")
			);
		}
		std::map<String,String> params;
		if(options.limit.has_value()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		if(options.offset.has_value()) {
			params["offset"] = std::to_string(options.offset.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/users/"+userId+"/playlists", params).map([](auto json) -> SpotifyPage<SpotifyPlaylist> {
			return SpotifyPage<SpotifyPlaylist>::fromJson(json);
		});
	}



	#pragma mark metadata: following artists

	Promise<void> Spotify::followArtists(ArrayList<String> artistIds) {
		return sendRequest(utils::HttpMethod::PUT, "v1/me/following", {
			{ "type", "artist" }
		}, Json::object{
			{ "ids", artistIds.map([](auto& artistId) { return Json((std::string)artistId); }) }
		}).toVoid();
	}

	Promise<void> Spotify::unfollowArtists(ArrayList<String> artistIds) {
		return sendRequest(utils::HttpMethod::DELETE, "v1/me/following", {
			{ "type", "artist" }
		}, Json::object{
			{ "ids", artistIds.map([](auto& artistId) { return Json((std::string)artistId); }) }
		}).toVoid();
	}

	Promise<ArrayList<bool>> Spotify::checkFollowingArtists(ArrayList<String> artistIds) {
		return sendRequest(utils::HttpMethod::GET, "v1/me/following/contains", {
			{ "type", "artist" }
		}, Json::object{
			{ "ids", artistIds.map([](auto& artistId) { return Json((std::string)artistId); }) }
		}).map([](auto json) -> ArrayList<bool> {
			ArrayList<bool> results;
			results.reserve(json.array_items().size());
			for(auto& jsonItem : json.array_items()) {
				results.pushBack(jsonItem.bool_value());
			}
			return results;
		});
	}

	Promise<SpotifyCursorPage<SpotifyArtist>> Spotify::getFollowedArtists(GetFollowedArtistsOptions options) {
		std::map<String,String> params = {
			{ "type", "artist" }
		};
		if(!options.after.empty()) {
			params["after"] = options.after;
		}
		if(options.limit.hasValue()) {
			params["limit"] = std::to_string(options.limit.value());
		}
		return sendRequest(utils::HttpMethod::GET, "v1/me/following", params).map([](auto json) -> SpotifyCursorPage<SpotifyArtist> {
			return SpotifyCursorPage<SpotifyArtist>::fromJson(json["artists"]);
		});
	}



	#pragma mark metadata: following artists

	Promise<void> Spotify::followUsers(ArrayList<String> userIds) {
		return sendRequest(utils::HttpMethod::PUT, "v1/me/following", {
			{ "type", "user" }
		}, Json::object{
			{ "ids", userIds.map([](auto& userId) { return Json((std::string)userId); }) }
		}).toVoid();
	}

	Promise<void> Spotify::unfollowUsers(ArrayList<String> userIds) {
		return sendRequest(utils::HttpMethod::DELETE, "v1/me/following", {
			{ "type", "user" }
		}, Json::object{
			{ "ids", userIds.map([](auto& userId) { return Json((std::string)userId); }) }
		}).toVoid();
	}

	Promise<ArrayList<bool>> Spotify::checkFollowingUsers(ArrayList<String> userIds) {
		return sendRequest(utils::HttpMethod::GET, "v1/me/following/contains", {
			{ "type", "user" }
		}, Json::object{
			{ "ids", userIds.map([](auto& userId) { return Json((std::string)userId); }) }
		}).map([](auto json) -> ArrayList<bool> {
			ArrayList<bool> results;
			results.reserve(json.array_items().size());
			for(auto& jsonItem : json.array_items()) {
				results.pushBack(jsonItem.bool_value());
			}
			return results;
		});
	}

	Promise<SpotifyPage<SpotifyUser>> Spotify::getFollowedUsers(GetFollowedUsersOptions options) {
		return Promise<SpotifyPage<SpotifyUser>>::reject(std::runtime_error("not implemented"));
	}
}
