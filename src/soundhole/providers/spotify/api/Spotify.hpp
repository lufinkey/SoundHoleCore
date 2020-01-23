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
#include "SpotifyMediaTypes.hpp"

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
		
		SpotifyAuth* getAuth();
		const SpotifyAuth* getAuth() const;
		SpotifyPlayer* getPlayer();
		const SpotifyPlayer* getPlayer() const;
		
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
		
		Promise<SpotifyUser> getMe();
		
		
		struct SearchOptions {
			ArrayList<String> types;
			String market;
			Optional<size_t> limit;
			Optional<size_t> offset;
		};
		Promise<SpotifySearchResults> search(String query, SearchOptions options = {});
		
		
		struct GetOptions {
			String market;
		};
		
		using GetAlbumOptions = GetOptions;
		Promise<SpotifyAlbum> getAlbum(String albumId, GetAlbumOptions options = {});
		using GetAlbumsOptions = GetOptions;
		Promise<ArrayList<SpotifyAlbum>> getAlbums(ArrayList<String> albumIds, GetAlbumsOptions options = {});
		struct GetAlbumTracksOptions {
			String market;
			Optional<size_t> limit;
			Optional<size_t> offset;
		};
		Promise<SpotifyPage<SpotifyTrack>> getAlbumTracks(String albumId, GetAlbumTracksOptions options = {});
		
		
		Promise<SpotifyArtist> getArtist(String artistId);
		Promise<ArrayList<SpotifyArtist>> getArtists(ArrayList<String> artistIds);
		struct GetArtistAlbumsOptions {
			String market;
			ArrayList<String> includeGroups;
			Optional<size_t> limit;
			Optional<size_t> offset;
		};
		Promise<SpotifyPage<SpotifyAlbum>> getArtistAlbums(String artistId, GetArtistAlbumsOptions options = {});
		using GetArtistTopTracksOptions = GetOptions;
		Promise<ArrayList<SpotifyTrack>> getArtistTopTracks(String artistId, String country, GetArtistTopTracksOptions options = {});
		Promise<ArrayList<SpotifyArtist>> getArtistRelatedArtists(String artistId);
		
		
		using GetTrackOptions = GetOptions;
		Promise<SpotifyTrack> getTrack(String trackId, GetTrackOptions options = {});
		using GetTracksOptions = GetOptions;
		Promise<ArrayList<SpotifyTrack>> getTracks(ArrayList<String> trackIds, GetTracksOptions options = {});
		Promise<Json> getTrackAudioAnalysis(String trackId);
		Promise<Json> getTrackAudioFeatures(String trackId);
		Promise<Json> getTracksAudioFeatures(ArrayList<String> trackIds);
		
		
		struct GetPlaylistOptions {
			String market;
			String fields;
		};
		Promise<SpotifyPlaylist> getPlaylist(String playlistId, GetPlaylistOptions options = {});
		struct GetPlaylistTracksOptions {
			String market;
			String fields;
			Optional<size_t> limit;
			Optional<size_t> offset;
		};
		Promise<SpotifyPage<SpotifyPlaylist::Item>> getPlaylistTracks(String playlistId, GetPlaylistTracksOptions options = {});
		
		
	private:
		Promise<void> prepareForPlayer();
		Promise<void> prepareForRequest();
		
		SpotifyPlayer::Options playerOptions;
		SpotifyAuth* auth;
		SpotifyPlayer* player;
		std::mutex playerMutex;
	};
}
