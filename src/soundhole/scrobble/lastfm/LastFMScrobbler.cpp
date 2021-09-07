//
//  LastFMScrobbler.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/6/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFMScrobbler.hpp"

namespace sh {
	Promise<ArrayList<ScrobbleResponse>> LastFMScrobbler::scrobble(ArrayList<ScrobbleRequest> scrobbles) {
		return lastfm->scrobble({
			.items = scrobbles.map([](auto& req) {
				return LastFMScrobbleRequest::Item{
					.track = req.track,
					.artist = req.artist,
					.album = req.album,
					.albumArtist = req.albumArtist,
					.timestamp = req.timestamp,
					.chosenByUser = req.chosenByUser,
					.trackNumber = req.trackNumber,
					.mbid = req.mbid,
					.duration = req.duration
				};
			})
		}).map(nullptr, [=](auto results) {
			return results.scrobbles.map([](auto& result) {
				return ScrobbleResponse{
					.track = {
						.text = result.track.text,
						.corrected = result.track.corrected
					},
					.artist = {
						.text = result.artist.text,
						.corrected = result.artist.corrected
					},
					.album = {
						.text = result.album.text,
						.corrected = result.album.corrected
					},
					.albumArtist = {
						.text = result.albumArtist.text,
						.corrected = result.albumArtist.corrected
					},
					.timestamp = result.timestamp,
					.ignored = (result.ignoredMessage.code != "0" && !result.ignoredMessage.code.trim().empty()) ? maybe(ScrobbleIgnored{
						.code = ignoredScrobbleCodeFromString(result.ignoredMessage.code),
						.message = result.ignoredMessage.text
					}) : std::nullopt
				};
			});
		});
	}

	ScrobbleIgnored::Code LastFMScrobbler::ignoredScrobbleCodeFromString(const String& str) {
		using Code = ScrobbleIgnored::Code;
		if(str == "1") {
			return Code::IGNORED_ARTIST;
		} else if(str == "2") {
			return Code::IGNORED_TRACK;
		} else if(str == "3") {
			return Code::TIMESTAMP_TOO_OLD;
		} else if(str == "4") {
			return Code::TIMESTAMP_TOO_NEW;
		} else if(str == "5") {
			return Code::DAILY_SCROBBLE_LIMIT_EXCEEDED;
		}
		return Code::UNKNOWN_ERROR;
	}


	Promise<void> LastFMScrobbler::loveTrack(String track, String artist) {
		return lastfm->loveTrack(track, artist);
	}

	Promise<void> LastFMScrobbler::unloveTrack(String track, String artist) {
		return lastfm->unloveTrack(track, artist);
	}
}
