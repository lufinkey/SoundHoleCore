//
//  Spotify.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/28/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Spotify.hpp"
#include <soundhole/utils/Utils.hpp>

namespace sh {
	Spotify::Spotify(Options options)
	: auth(new SpotifyAuth(options)), player(nullptr) {
		//
	}
	
	Promise<bool> Spotify::login() {
		return auth->login();
	}
	
	Promise<void> Spotify::logout() {
		auto player = this->player;
		auth->logout();
		if(player != nullptr) {
			return player->logout();
		}
		return Promise<void>::resolve();
	}
	
	Promise<void> Spotify::startPlayer() {
		if(!auth->hasStreamingScope()) {
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
			auto player = SpotifyPlayer::shared;
			if(player->getAuth() != nullptr && player->getAuth() != this->auth) {
				return Promise<void>::reject(
					SpotifyError(SpotifyError::Code::PLAYER_IN_USE, "Player is being used by another SpotifyAuth instance")
				);
			}
			player->setAuth(this->auth);
			this->player = player;
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
			player->logout();
			player->stop();
			player = nullptr;
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
			return setPlaying(playing);
		});
	}
	
	Promise<void> Spotify::setShuffling(bool shuffling) {
		return prepareForPlayer().then([=]() -> Promise<void> {
			return setShuffling(shuffling);
		});
	}
	
	Promise<void> Spotify::setRepeating(bool repeating) {
		return prepareForPlayer().then([=]() -> Promise<void> {
			return setRepeating(repeating);
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
}
