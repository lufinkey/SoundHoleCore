//
//  YoutubeProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include "api/Youtube.hpp"

namespace sh {
	class YoutubeProvider: public MediaProvider {
	public:
		YoutubeProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		
		virtual Promise<Track::Data> getTrackData(String uri) override;
		virtual Promise<Artist::Data> getArtistData(String uri) override;
		virtual Promise<Album::Data> getAlbumData(String uri) override;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) override;
		
		virtual MediaPlaybackProvider* player() override;
		virtual const MediaPlaybackProvider* player() const override;
			
	protected:
		Track::Data createTrackData(YoutubeVideo video);
		Track::Data createTrackData(YoutubeVideoInfo video);
		Artist::Data createArtistData(YoutubeChannel channel);
		Playlist::Data createPlaylistData(YoutubePlaylist playlist);
		
	private:
		struct URI {
			String provider;
			String type;
			String id;
		};
		URI parseURI(String uri);
		URI parseURL(String url);
		String createURI(String type, String id);
		
		static MediaItem::Image createImage(YoutubeImage image);
		
		Youtube* youtube;
	};
}
