//
//  Scrobbler.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/6/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/Track.hpp>

namespace sh {
	struct ScrobbleRequest {
		String track;
		String artist;
		String album;
		String albumArtist;
		Optional<size_t> trackNumber;
		Optional<double> duration;
		String musicBrainzId;
		Date timestamp;
		Optional<bool> chosenByUser;
	};

	struct ScrobbleIgnored {
		enum Code {
			IGNORED_ARTIST,
			IGNORED_TRACK,
			TIMESTAMP_TOO_OLD,
			TIMESTAMP_TOO_NEW,
			DAILY_SCROBBLE_LIMIT_EXCEEDED,
			UNKNOWN_ERROR
		};
		
		Code code;
		String message;
	};

	struct ScrobbleResponse {
		struct Property {
			Optional<String> text;
			bool corrected;
		};
		
		Property track;
		Property artist;
		Property album;
		Property albumArtist;
		Date timestamp;
		
		Optional<ScrobbleIgnored> ignored;
	};

	struct TrackLoveRequest {
		String track;
		String artist;
		String musicBrainzId;
	};

	class Scrobbler {
	public:
		virtual ~Scrobbler() {}
		
		virtual String name() const = 0;
		virtual String displayName() const = 0;
		
		virtual Promise<ArrayList<ScrobbleResponse>> scrobble(ArrayList<ScrobbleRequest> scrobbles) = 0;
		
		virtual Promise<void> loveTrack(TrackLoveRequest) = 0;
		virtual Promise<void> unloveTrack(TrackLoveRequest) = 0;
	};
}
