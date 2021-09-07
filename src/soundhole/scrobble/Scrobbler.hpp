//
//  Scrobbler.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/6/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	struct ScrobbleRequest {
		String track;
		String artist;
		String album;
		String albumArtist;
		Date timestamp;
		Optional<bool> chosenByUser;
		Optional<size_t> trackNumber;
		String mbid;
		Optional<double> duration;
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

	class Scrobbler {
	public:
		virtual ~Scrobbler() {}
		
		virtual Promise<ArrayList<ScrobbleResponse>> scrobble(ArrayList<ScrobbleRequest> scrobbles) = 0;
		
		virtual Promise<void> loveTrack(String track, String artist) = 0;
		virtual Promise<void> unloveTrack(String track, String artist) = 0;
	};
}
