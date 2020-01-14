//
//  YoutubeProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "YoutubeProvider.hpp"
#include <soundhole/utils/SoundHoleError.hpp>

namespace sh {
	YoutubeProvider::YoutubeProvider(Options options)
	: youtube(new Youtube(options)), _player(new YoutubePlaybackProvider(this)) {
		//
	}

	YoutubeProvider::~YoutubeProvider() {
		delete _player;
		delete youtube;
	}
	
	String YoutubeProvider::name() const {
		return "youtube";
	}
	
	String YoutubeProvider::displayName() const {
		return "YouTube";
	}




	Promise<bool> YoutubeProvider::login() {
		// TODO implement login
		return Promise<bool>::reject(std::logic_error("Youtube login is not available"));
	}

	void YoutubeProvider::logout() {
		// TODO implement logout
	}

	bool YoutubeProvider::isLoggedIn() const {
		return false;
	}




	Track::Data YoutubeProvider::createTrackData(YoutubeVideo video) {
		return Track::Data{{
			.type="track",
			.name=video.snippet.title,
			.uri=createURI("video", video.id),
			.images=video.snippet.thumbnails.map<MediaItem::Image>([&](auto image) {
				return createImage(image);
			})
			},
			.albumName="",
			.albumURI="",
			.artists=ArrayList<$<Artist>>{
				Artist::new$(this, Artist::Data{{
					.type="artist",
					.name=video.snippet.channelTitle,
					.uri=createURI("channel", video.snippet.channelId),
					.images=std::nullopt
					},
					.description=std::nullopt
				})
			},
			.tags=video.snippet.tags,
			.discNumber=std::nullopt,
			.trackNumber=std::nullopt,
			.duration=std::nullopt,
			.audioSources=std::nullopt
		};
	}

	Track::Data YoutubeProvider::createTrackData(YoutubeVideoInfo video) {
		String title = video.title;
		String videoId = video.videoId;
		if((title.empty() || videoId.empty()) && video.playerResponse && video.playerResponse->videoDetails) {
			if(title.empty()) {
				if(!video.playerResponse->videoDetails->title.empty()) {
					title = video.playerResponse->videoDetails->title;
				}
			}
			if(videoId.empty()) {
				if(!video.playerResponse->videoDetails->videoId.empty()) {
					videoId = video.playerResponse->videoDetails->videoId;
				}
			}
		}
		if(videoId.empty()) {
			throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "ytdl response is missing crucial video information");
		}
		return Track::Data{{
			.type="track",
			.name=title,
			.uri=createURI("video", videoId),
			.images=([&]() -> Optional<ArrayList<MediaItem::Image>> {
				if(!video.playerResponse || !video.playerResponse->videoDetails || !video.playerResponse->videoDetails->thumbnail) {
					return std::nullopt;
				}
				auto images = video.playerResponse->videoDetails->thumbnail->thumbnails.map<MediaItem::Image>([&](auto image) {
					auto dimensions = MediaItem::Image::Dimensions{
						.width=image.width,
						.height=image.height
					};
					return MediaItem::Image{
						.url=image.url,
						.size=dimensions.toSize(),
						.dimensions=dimensions
					};
				});
				if(!video.thumbnailURL.empty()) {
					images.pushBack(MediaItem::Image{
						.url=video.thumbnailURL,
						.size=MediaItem::Image::Size::SMALL,
						.dimensions=std::nullopt
					});
				}
				return images;
			})(),
			},
			.albumName="",
			.albumURI="",
			.artists=([&]() {
				ArrayList<$<Artist>> artists;
				if(video.author && !video.author->id.empty()) {
					artists.pushBack(Artist::new$(this, Artist::Data{{
						.type="artist",
						.name=video.author->name,
						.uri=createURI("channel", video.playerResponse->videoDetails->channelId),
						.images=([&]() -> Optional<ArrayList<MediaItem::Image>> {
							if(video.author->avatar.empty()) {
								return std::nullopt;
							}
							return ArrayList<MediaItem::Image>{
								MediaItem::Image{
									.url=video.author->avatar,
									.size=MediaItem::Image::Size::SMALL,
									.dimensions=std::nullopt
								}
							};
						})()
						},
						.description=std::nullopt
					}));
				} else if(video.playerResponse && video.playerResponse->videoDetails && !video.playerResponse->videoDetails->channelId.empty()) {
					artists.pushBack(Artist::new$(this, Artist::Data{{
						.type="artist",
						.name=video.playerResponse->videoDetails->author,
						.uri=createURI("channel", video.playerResponse->videoDetails->channelId),
						.images=std::nullopt
						},
						.description=std::nullopt
					}));
				}
				if(video.media) {
					String artistURI;
					if(!video.media->artistURL.empty()) {
						try {
							auto uriParts = parseURL(video.media->artistURL);
							if(uriParts.type != "channel") {
								throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "Youtube artistURL is not a valid channel");
							}
							artistURI = createURI(uriParts.type, uriParts.id);
						} catch(SoundHoleError&) {
							// do nothing
						}
					}
					if(artistURI.empty() || !artists.containsWhere([&](auto& cmpArtist) { return (artistURI == cmpArtist->uri()); })) {
						artists.pushBack(Artist::new$(this, Artist::Data{{
							.type="artist",
							.name=video.playerResponse->videoDetails->author,
							.uri=createURI("channel", video.playerResponse->videoDetails->channelId),
							.images=std::nullopt
							},
							.description=std::nullopt
						}));
					}
				}
				return artists;
			})(),
			.tags=(video.playerResponse && video.playerResponse->videoDetails) ?
				maybe(video.playerResponse->videoDetails->keywords)
				: std::nullopt,
			.discNumber=std::nullopt,
			.trackNumber=std::nullopt,
			.duration=(video.playerResponse && video.playerResponse->videoDetails) ?
				maybeTry([&]() { return video.playerResponse->videoDetails->lengthSeconds.toArithmeticValue<double>(); })
				: std::nullopt,
			.audioSources=video.formats.where([&](auto& format) {
				return format.audioBitrate.has_value();
			}).map<Track::AudioSource>([&](auto& format) {
				return Track::AudioSource{
					.url=format.url,
					.encoding=format.audioEncoding,
					.bitrate=format.audioBitrate.value(),
					.videoBitrate=([&]() -> Optional<double> {
						auto bitrateParts = format.bitrate.split('-');
						if(bitrateParts.size() == 0) {
							return std::nullopt;
						}
						return maybeTry([&]{ return bitrateParts.back().template toArithmeticValue<double>(); });
					})()
				};
			})
		};
	}

	Artist::Data YoutubeProvider::createArtistData(YoutubeChannel channel) {
		return Artist::Data{{
			.type="artist",
			.name=channel.snippet.title,
			.uri=createURI("channel", channel.id),
			.images=channel.snippet.thumbnails.map<MediaItem::Image>([&](auto image) {
				return createImage(image);
			})
			},
			.description=channel.snippet.description
		};
	}

	Playlist::Data YoutubeProvider::createPlaylistData(YoutubePlaylist playlist) {
		return Playlist::Data{{{
			.type="playlist",
			.name=playlist.snippet.title,
			.uri=createURI("playlist", playlist.id),
			.images=playlist.snippet.thumbnails.map<MediaItem::Image>([&](auto image) {
				return createImage(image);
			})
			},
			.tracks=std::nullopt,
			},
			.owner=UserAccount::new$(this, UserAccount::Data{{
				.type="user",
				.name=playlist.snippet.channelTitle,
				.uri=createURI("channel", playlist.snippet.channelId)
				},
				.id=playlist.snippet.channelId,
				.displayName=playlist.snippet.channelTitle
			})
		};
	}

	MediaItem::Image YoutubeProvider::createImage(YoutubeImage image) {
		return MediaItem::Image{
			.url=image.url,
			.size=([&]() {
				switch (image.size) {
					case YoutubeImage::Size::DEFAULT:
						return MediaItem::Image::Size::TINY;
					case YoutubeImage::Size::MEDIUM:
						return MediaItem::Image::Size::SMALL;
					case YoutubeImage::Size::HIGH:
					case YoutubeImage::Size::STANDARD:
						return MediaItem::Image::Size::MEDIUM;
					case YoutubeImage::Size::MAXRES:
						return MediaItem::Image::Size::LARGE;
				}
			})(),
			.dimensions=([&]() -> Optional<MediaItem::Image::Dimensions> {
				if(!image.dimensions) {
					return std::nullopt;
				}
				return MediaItem::Image::Dimensions{
					.width=image.dimensions->width,
					.height=image.dimensions->height
				};
			})()
		};
	}




	Promise<Track::Data> YoutubeProvider::getTrackData(String uri) {
		auto uriParts = parseURI(uri);
		if(uriParts.type != "video") {
			throw std::invalid_argument("invalid uri type "+uriParts.type);
		}
		return youtube->getVideoInfo(uriParts.id).map<Track::Data>([=](auto track) {
			return createTrackData(track);
		});
	}

	Promise<Artist::Data> YoutubeProvider::getArtistData(String uri) {
		auto uriParts = parseURI(uri);
		if(uriParts.type == "channel") {
			throw std::invalid_argument("invalid uri type "+uriParts.type);
		}
		return youtube->getChannel(uri).map<Artist::Data>([=](auto channel) {
			return createArtistData(channel);
		});
	}

	Promise<Album::Data> YoutubeProvider::getAlbumData(String uri) {
		return Promise<Album::Data>::reject(std::logic_error("Youtube does not support albums"));
	}

	Promise<Playlist::Data> YoutubeProvider::getPlaylistData(String uri) {
		return youtube->getPlaylist(uri).map<Playlist::Data>([=](auto playlist) {
			return createPlaylistData(playlist);
		});
	}



	YoutubePlaybackProvider* YoutubeProvider::player() {
		return _player;
	}

	const YoutubePlaybackProvider* YoutubeProvider::player() const {
		return _player;
	}
}
