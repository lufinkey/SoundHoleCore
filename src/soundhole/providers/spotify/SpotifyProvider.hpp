//
//  SpotifyProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/media/MediaProvider.hpp>
#include "api/Spotify.hpp"

namespace sh {
	class SpotifyProvider: public MediaProvider {
	public:
		using Options = Spotify::Options;
		
		SpotifyProvider(Options options);
		virtual ~SpotifyProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		bool isLoggedIn() const override;
		
		virtual Promise<Track::Data> getTrackData(String uri) override;
		virtual Promise<Artist::Data> getArtistData(String uri) override;
		virtual Promise<Album::Data> getAlbumData(String uri) override;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) override;
		
		virtual bool usesPublicAudioStreams() const override;
		
	protected:
		Track::Data createTrackData(SpotifyTrack track);
		Artist::Data createArtistData(SpotifyArtist artist);
		Album::Data createAlbumData(SpotifyAlbum album);
		Playlist::Data createPlaylistData(SpotifyPlaylist playlist);
		UserAccount::Data createUserAccountData(SpotifyUser user);
		
	private:
		static String idFromURI(String uri);
		static MediaItem::Image createImage(SpotifyImage image);
		
		Spotify* spotify;
	};
}
