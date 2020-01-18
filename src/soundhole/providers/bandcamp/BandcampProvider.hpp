//
//  BandcampProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include "BandcampPlaybackProvider.hpp"
#include "api/Bandcamp.hpp"

namespace sh {
	class BandcampProvider: public MediaProvider {
		friend class BandcampAlbumMutatorDelegate;
	public:
		BandcampProvider();
		virtual ~BandcampProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		
		virtual Promise<Track::Data> getTrackData(String uri) override;
		virtual Promise<Artist::Data> getArtistData(String uri) override;
		virtual Promise<Album::Data> getAlbumData(String uri) override;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) override;
		
		virtual Album::MutatorDelegate* createAlbumMutatorDelegate($<Album> album) override;
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) override;
		
		virtual BandcampPlaybackProvider* player() override;
		virtual const BandcampPlaybackProvider* player() const override;
		
	protected:
		Track::Data createTrackData(BandcampTrack track);
		Artist::Data createArtistData(BandcampArtist artist);
		Album::Data createAlbumData(BandcampAlbum album);
		
	private:
		static MediaItem::Image createImage(BandcampImage image);
		
		Bandcamp* bandcamp;
		BandcampPlaybackProvider* _player;
	};
}
