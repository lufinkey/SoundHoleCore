//
//  SpotifyPlaybackProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyPlaybackProvider.hpp"
#include "SpotifyProvider.hpp"

namespace sh {
	SpotifyPlaybackProvider::SpotifyPlaybackProvider(SpotifyProvider* provider)
	: provider(provider) {
		provider->spotify->addPlayerEventListener(this);
	}

	SpotifyPlaybackProvider::~SpotifyPlaybackProvider() {
		provider->spotify->removePlayerEventListener(this);
	}



	bool SpotifyPlaybackProvider::usesPublicAudioStreams() const {
		return false;
	}
	
	Promise<void> SpotifyPlaybackProvider::prepare($<Track> track) {
		return track->fetchDataIfNeeded();
	}

	Promise<void> SpotifyPlaybackProvider::play($<Track> track, double position) {
		playQueue.cancelAllTasks();
		setPlayingQueue.cancelAllTasks();
		return playQueue.run([=](auto task) {
			return generate_items<void>({
				[=]() {
					return track->fetchDataIfNeeded();
				},
				[=]() {
					return provider->spotify->playURI(track->uri(), {
						.position=position
					});
				},
				[=]() {
					setPlayingUntilSuccess();
					return Promise<void>::resolve();
				}
			});
		}).promise;
	}

	Promise<void> SpotifyPlaybackProvider::setPlaying(bool playing) {
		if(!playing && !provider->spotify->getPlaybackState().playing) {
			setPlayingQueue.cancelAllTasks();
		}
		return provider->spotify->setPlaying(playing);
	}

	void SpotifyPlaybackProvider::stop() {
		playQueue.cancelAllTasks();
		setPlayingQueue.cancelAllTasks();
		provider->spotify->stopPlayer();
	}

	Promise<void> SpotifyPlaybackProvider::seek(double position) {
		return provider->spotify->seek(position);
	}
	
	SpotifyPlaybackProvider::State SpotifyPlaybackProvider::state() const {
		auto spotifyState = provider->spotify->getPlaybackState();
		return State{
			.playing=spotifyState.playing,
			.position=spotifyState.position,
			.shuffling=spotifyState.shuffling,
			.repeating=spotifyState.repeating
		};
	}

	SpotifyPlaybackProvider::Metadata SpotifyPlaybackProvider::metadata() const {
		auto spotifyMetadata = provider->spotify->getPlaybackMetadata();
		auto& prevTrack = spotifyMetadata.previousTrack;
		auto& currentTrack = spotifyMetadata.currentTrack;
		auto& nextTrack = spotifyMetadata.nextTrack;
		return Metadata{
			.previousTrack=prevTrack.has_value() ?
				Track::new$(provider, createTrackData(std::move(prevTrack.value())))
				: nullptr,
			.currentTrack=currentTrack.has_value() ?
				Track::new$(provider, createTrackData(std::move(currentTrack.value())))
				: nullptr,
			.nextTrack=nextTrack.has_value() ?
				Track::new$(provider, createTrackData(std::move(nextTrack.value())))
				: nullptr,
		};
	}



	Promise<void> SpotifyPlaybackProvider::setPlayingUntilSuccess() {
		return setPlayingQueue.run([=](auto task) {
			return generate<void>([=](auto yield) {
				while(true) {
					try {
						await(provider->spotify->setPlaying(true));
						yield();
						if(!provider->spotify->getPlaybackState().playing) {
							// wait for player to start playing
							for(size_t i=0; i<10; i++) {
								await(Promise<void>::resolve().delay(std::chrono::milliseconds(200)));
								if(provider->spotify->getPlaybackState().playing) {
									// hooray, it started playing!
									break;
								}
								yield();
							}
							if(!provider->spotify->getPlaybackState().playing) {
								throw std::runtime_error("timed out waiting for player to play");
							}
						}
						return;
					} catch(GenerateDestroyedNotifier&) {
						std::rethrow_exception(std::current_exception());
					} catch(Error& error) {
						printf("Error attempting to play Spotify player: %s", error.toString().c_str());
						// continue trying...
					} catch(std::exception& error) {
						printf("Error attempting to play Spotify player: %s", error.what());
						// continue trying...
					}
					yield();
				}
			});
		}).promise;
	}



	Track::Data SpotifyPlaybackProvider::createTrackData(SpotifyPlayer::Track track) const {
		return Track::Data{{
			.partial=true,
			.type="track",
			.name=track.name,
			.uri=track.uri,
			.images=track.albumCoverArtURL ? maybe(ArrayList<MediaItem::Image>{
				MediaItem::Image{
					.url=track.albumCoverArtURL.value(),
					.size=MediaItem::Image::Size::MEDIUM,
					.dimensions=std::nullopt
				}
			}) : std::nullopt
			},
			.albumName=track.albumName,
			.albumURI=track.albumURI,
			.artists=ArrayList<$<Artist>>{
				Artist::new$(provider, Artist::Data{{
					.partial=true,
					.type="artist",
					.name=track.artistName,
					.uri=track.artistURI,
					.images=std::nullopt
					},
					.description=std::nullopt
				})
			},
			.tags=std::nullopt,
			.discNumber=std::nullopt,
			.trackNumber=std::nullopt,
			.duration=track.duration,
			.audioSources=std::nullopt,
			.playable=true
		};
	}



#pragma mark SpotifyPlayerEventListener

	void SpotifyPlaybackProvider::onSpotifyPlaybackEvent(SpotifyPlayer* player, SpotifyPlaybackEvent event) {
		if(event == SpotifyPlaybackEvent::PLAY) {
			callListenerEvent(&EventListener::onMediaPlaybackProviderPlay, this);
		} else if(event == SpotifyPlaybackEvent::PAUSE) {
			callListenerEvent(&EventListener::onMediaPlaybackProviderPause, this);
		} else if(event == SpotifyPlaybackEvent::TRACK_DELIVERED) {
			callListenerEvent(&EventListener::onMediaPlaybackProviderTrackFinish, this);
		} else if(event == SpotifyPlaybackEvent::METADATA_CHANGE) {
			callListenerEvent(&EventListener::onMediaPlaybackProviderMetadataChange, this);
		}
	}
}
