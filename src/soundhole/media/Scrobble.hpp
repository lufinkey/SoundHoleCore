//
//  Scrobble.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/12/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Scrobble.hpp"

namespace sh {
	class Scrobbler;
	class ScrobblerStash;
	class ScrobbleManager;
	struct UnmatchedScrobble;

	class Scrobble: public std::enable_shared_from_this<Scrobble> {
		friend class ScrobbleManager;
	public:
		struct IgnoredReason {
			enum Code {
				IGNORED_ARTIST,
				IGNORED_TRACK,
				TIMESTAMP_TOO_OLD,
				TIMESTAMP_TOO_NEW,
				DAILY_SCROBBLE_LIMIT_EXCEEDED,
				UNKNOWN_ERROR
			};
			static Code Code_fromString(const String&);
			static String Code_toString(const Code&);
			
			Code code;
			String message;
			
			static IgnoredReason fromJson(const Json&);
			Json toJson() const;
		};
		
		struct Response {
			struct Property {
				Optional<String> text;
				bool corrected;
			};
			
			Property track;
			Property artist;
			Property album;
			Property albumArtist;
			Date timestamp;
			
			Optional<Scrobble::IgnoredReason> ignored;
		};
		
		struct Data {
			String localID;
			Scrobbler* scrobbler;
			Date startTime;
			String trackURI;
			String musicBrainzID;
			String trackName;
			String artistName;
			String albumName;
			String albumArtistName;
			Optional<double> duration;
			Optional<size_t> trackNumber;
			Optional<bool> chosenByUser;
			Optional<Date> historyItemStartTime;
			bool uploaded;
			Optional<IgnoredReason> ignoredReason;
			
			static Data fromJson(const Json& json, ScrobblerStash* stash);
		};
		
		static $<Scrobble> new$(Data);
		
		Scrobble(Data);
		~Scrobble();
		
		const String& localID() const;
		
		Scrobbler* scrobbler();
		const Scrobbler* scrobbler() const;
		
		const Date& startTime() const;
		const String& trackURI() const;
		const String& musicBrainzID() const ;
		const String& trackName() const;
		const String& artistName() const;
		const String& albumName() const;
		const String& albumArtistName() const;
		const Optional<double>& duration() const;
		const Optional<size_t>& trackNumber() const;
		const Optional<bool>& chosenByUser() const;
		const Optional<Date>& historyItemStartTime() const;
		bool isUploaded() const;
		const Optional<IgnoredReason>& ignoredReason() const;
		
		void applyData(Data);
		void applyResponse(const Response&);
		void matchWith(const UnmatchedScrobble&);
		
		Data toData() const;
		
		static $<Scrobble> fromJson(const Json& json, ScrobblerStash* stash);
		Json toJson() const;
		
	private:
		String _localID;
		Scrobbler* _scrobbler;
		Date _startTime;
		String _trackURI;
		String _musicBrainzID;
		String _trackName;
		String _artistName;
		String _albumName;
		String _albumArtistName;
		Optional<double> _duration;
		Optional<size_t> _trackNumber;
		Optional<bool> _chosenByUser;
		Optional<Date> _historyItemStartTime;
		bool _uploaded;
		Optional<IgnoredReason> _ignoredReason;
	};
}
