//
//  Spotify.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/28/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/HttpClient.hpp>
#include "SpotifyAuth.hpp"
#include "SpotifyPlayer.hpp"

namespace sh {
	class Spotify {
	public:
		struct Options {
			SpotifyAuth::Options auth;
			SpotifyPlayer::Options player;
		};
		
		Spotify(const Spotify&) = delete;
		Spotify& operator=(const Spotify&) = delete;
		
		Spotify(Options options);
		~Spotify();
		
		using LoginOptions = SpotifyAuth::LoginOptions;
		Promise<bool> login(LoginOptions options = LoginOptions());
		Promise<void> logout();
		bool isLoggedIn() const;
		
		Promise<void> startPlayer();
		void stopPlayer();
		
		using PlayOptions = SpotifyPlayer::PlayOptions;
		Promise<void> playURI(String uri, PlayOptions options = {.index=0,.position=0});
		Promise<void> queueURI(String uri);
		
		Promise<void> skipToNext();
		Promise<void> skipToPrevious();
		Promise<void> seek(double position);
		
		Promise<void> setPlaying(bool playing);
		Promise<void> setShuffling(bool shuffling);
		Promise<void> setRepeating(bool repeating);
		
		SpotifyPlayer::State getPlaybackState() const;
		SpotifyPlayer::Metadata getPlaybackMetadata() const;
		
		Promise<Json> sendRequest(utils::HttpMethod method, String endpoint, Json params = Json());
		
		Promise<Json> getMe(Json options = Json());
		
		Promise<Json> search(String query, ArrayList<String> types, Json options = Json());
		
		Promise<Json> getAlbum(String albumId, Json options = Json());
		Promise<Json> getAlbums(ArrayList<String> albumIds, Json options = Json());
		Promise<Json> getAlbumTracks(String albumId, Json options = Json());
		
		Promise<Json> getArtist(String artistId, Json options = Json());
		Promise<Json> getArtists(ArrayList<String> artistIds, Json options = Json());
		Promise<Json> getArtistAlbums(String artistId, Json options = Json());
		Promise<Json> getArtistTopTracks(String artistId, String country, Json options = Json());
		Promise<Json> getArtistRelatedArtists(String artistId, Json options = Json());
		
		Promise<Json> getTrack(String trackId, Json options = Json());
		Promise<Json> getTracks(ArrayList<String> trackIds, Json options = Json());
		Promise<Json> getTrackAudioAnalysis(String trackId, Json options = Json());
		Promise<Json> getTrackAudioFeatures(String trackId, Json options = Json());
		Promise<Json> getTracksAudioFeatures(ArrayList<String> trackIds, Json options = Json());
		
		Promise<Json> getPlaylist(String playlistId, Json options = Json());
		Promise<Json> getPlaylistTracks(String playlistId, Json options = Json());
		
	private:
		Promise<void> prepareForPlayer();
		Promise<void> prepareForRequest();
		
		SpotifyPlayer::Options playerOptions;
		SpotifyAuth* auth;
		SpotifyPlayer* player;
		std::mutex playerMutex;
	};
}
