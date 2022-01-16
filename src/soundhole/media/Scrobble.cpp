//
//  Scrobble.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/12/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include "Scrobble.hpp"
#include <soundhole/media/Scrobbler.hpp>
#include <soundhole/media/ScrobblerStash.hpp>

namespace sh {
	Scrobble::IgnoredReason::Code Scrobble::IgnoredReason::Code_fromString(const String& str) {
		if(str == "IGNORED_ARTIST") {
			return Code::IGNORED_ARTIST;
		} else if(str == "IGNORED_TRACK") {
			return Code::IGNORED_TRACK;
		} else if(str == "TIMESTAMP_TOO_OLD") {
			return Code::TIMESTAMP_TOO_OLD;
		} else if(str == "TIMESTAMP_TOO_NEW") {
			return Code::TIMESTAMP_TOO_NEW;
		} else if(str == "DAILY_SCROBBLE_LIMIT_EXCEEDED") {
			return Code::DAILY_SCROBBLE_LIMIT_EXCEEDED;
		} else if(str == "UNKNOWN_ERROR") {
			return Code::UNKNOWN_ERROR;
		}
		throw std::invalid_argument("invalid Scrobble::IgnoredReason::Code "+str);
	}

	String Scrobble::IgnoredReason::Code_toString(const Code& code) {
		switch(code) {
			case Code::IGNORED_ARTIST:
				return "IGNORED_ARTIST";
				
			case Code::IGNORED_TRACK:
				return "IGNORED_TRACK";
				
			case Code::TIMESTAMP_TOO_OLD:
				return "TIMESTAMP_TOO_OLD";
				
			case Code::TIMESTAMP_TOO_NEW:
				return "TIMESTAMP_TOO_NEW";
				
			case Code::DAILY_SCROBBLE_LIMIT_EXCEEDED:
				return "DAILY_SCROBBLE_LIMIT_EXCEEDED";
				
			case Code::UNKNOWN_ERROR:
				return "UNKNOWN_ERROR";
		}
		throw std::invalid_argument("invalid Scrobble::IgnoredReason::Code value "+std::to_string((int)code));
	}

	Scrobble::IgnoredReason Scrobble::IgnoredReason::fromJson(const Json& json) {
		return IgnoredReason{
			.code = Code_fromString(json["code"].string_value()),
			.message = json["message"].string_value()
		};
	}

	Json Scrobble::IgnoredReason::toJson() const {
		return Json::object{
			{ "code", (std::string)Code_toString(code) },
			{ "message", (std::string)message }
		};
	}

	Scrobble::Data Scrobble::Data::fromJson(const Json& json, ScrobblerStash* stash) {
		auto durationJson = json["duration"];
		auto trackNumJson = json["trackNumber"];
		auto chosenByUserJson = json["chosenByUser"];
		auto historyItemStartTimeJson = json["historyItemStartTime"];
		auto ignoredReasonJson = json["ignoredReason"];
		return Scrobble::Data{
			.localID = json["localID"].string_value(),
			.scrobbler = stash->getScrobbler(json["scrobbler"].string_value()),
			.startTime = Date::fromISOString(json["startTime"].string_value()),
			.trackURI = json["trackURI"].string_value(),
			.musicBrainzID = json["musicBrainzID"].string_value(),
			.trackName = json["trackName"].string_value(),
			.artistName = json["artistName"].string_value(),
			.albumName = json["albumName"].string_value(),
			.albumArtistName = json["albumArtistName"].string_value(),
			.duration = durationJson.is_number() ? maybe(durationJson.number_value()) : std::nullopt,
			.trackNumber = trackNumJson.is_number() ? maybe((size_t)trackNumJson.number_value()) : std::nullopt,
			.chosenByUser = chosenByUserJson.is_bool() ? maybe(chosenByUserJson.bool_value()) : std::nullopt,
			.historyItemStartTime = historyItemStartTimeJson.is_string() ? Date::maybeFromISOString(historyItemStartTimeJson.string_value()) : std::nullopt,
			.uploaded = json["uploaded"].bool_value(),
			.ignoredReason = ignoredReasonJson.is_object() ? maybe(IgnoredReason::fromJson(ignoredReasonJson)) : std::nullopt
		};
	}



	$<Scrobble> Scrobble::new$(Data data) {
		return fgl::new$<Scrobble>(data);
	}

	Scrobble::Scrobble(Data data)
	: _localID(data.localID),
	_scrobbler(data.scrobbler),
	_startTime(data.startTime),
	_trackURI(data.trackURI),
	_musicBrainzID(data.musicBrainzID),
	_trackName(data.trackName),
	_artistName(data.artistName),
	_albumName(data.albumName),
	_albumArtistName(data.albumArtistName),
	_duration(data.duration),
	_trackNumber(data.trackNumber),
	_chosenByUser(data.chosenByUser),
	_historyItemStartTime(data.historyItemStartTime),
	_uploaded(data.uploaded),
	_ignoredReason(data.ignoredReason) {
		//
	}

	Scrobble::~Scrobble() {
		//
	}

	const String& Scrobble::localID() const {
		return _localID;
	}

	Scrobbler* Scrobble::scrobbler() {
		return _scrobbler;
	}

	const Scrobbler* Scrobble::scrobbler() const {
		return _scrobbler;
	}

	const Date& Scrobble::startTime() const {
		return _startTime;
	}

	const String& Scrobble::trackURI() const {
		return _trackURI;
	}

	const String& Scrobble::musicBrainzID() const {
		return _musicBrainzID;
	}

	const String& Scrobble::trackName() const {
		return _trackName;
	}

	const String& Scrobble::artistName() const {
		return _artistName;
	}

	const String& Scrobble::albumName() const {
		return _albumName;
	}

	const String& Scrobble::albumArtistName() const {
		return _albumArtistName;
	}

	const Optional<double>& Scrobble::duration() const {
		return _duration;
	}

	const Optional<size_t>& Scrobble::trackNumber() const {
		return _trackNumber;
	}

	const Optional<bool>& Scrobble::chosenByUser() const {
		return _chosenByUser;
	}

	const Optional<Date>& Scrobble::historyItemStartTime() const {
		return _historyItemStartTime;
	}

	bool Scrobble::isUploaded() const {
		return _uploaded;
	}

	const Optional<Scrobble::IgnoredReason>& Scrobble::ignoredReason() const {
		return _ignoredReason;
	}



	void Scrobble::applyData(Data data) {
		if(!data.localID.empty()) {
			_localID = data.localID;
		}
		_startTime = data.startTime;
		if(!data.trackURI.empty()) {
			_trackURI = data.trackURI;
		}
		if(!data.musicBrainzID.empty()) {
			_musicBrainzID = data.musicBrainzID;
		}
		if(!data.trackName.empty()) {
			_trackName = data.trackName;
		}
		if(!data.artistName.empty()) {
			_artistName = data.artistName;
		}
		if(!data.albumName.empty()) {
			_albumName = data.albumName;
		}
		if(!data.albumArtistName.empty()) {
			_albumArtistName = data.albumArtistName;
		}
		if(data.duration.hasValue()) {
			_duration = data.duration;
		}
		if(data.trackNumber.hasValue()) {
			_trackNumber = data.trackNumber;
		}
		if(data.chosenByUser.hasValue()) {
			_chosenByUser = data.chosenByUser;
		}
		if(data.historyItemStartTime.hasValue()) {
			_historyItemStartTime = data.historyItemStartTime;
		}
		_uploaded = data.uploaded;
		_ignoredReason = data.ignoredReason;
	}

	void Scrobble::applyResponse(const Response& response) {
		if(response.ignored
		   && (response.ignored->code == IgnoredReason::Code::DAILY_SCROBBLE_LIMIT_EXCEEDED
			   || response.ignored->code == IgnoredReason::Code::TIMESTAMP_TOO_NEW)) {
			_uploaded = false;
		} else {
			_uploaded = true;
		}
		_ignoredReason = response.ignored;
	}

	Scrobble::Data Scrobble::toData() const {
		return Data{
			.localID = _localID,
			.scrobbler = _scrobbler,
			.startTime = _startTime,
			.trackURI = _trackURI,
			.musicBrainzID = _musicBrainzID,
			.trackName = _trackName,
			.artistName = _artistName,
			.albumName = _albumName,
			.albumArtistName = _albumArtistName,
			.duration = _duration,
			.trackNumber = _trackNumber,
			.chosenByUser = _chosenByUser,
			.historyItemStartTime = _historyItemStartTime,
			.uploaded = _uploaded,
			.ignoredReason = _ignoredReason
		};
	}

	$<Scrobble> Scrobble::fromJson(const Json& json, ScrobblerStash* stash) {
		return Scrobble::new$(Scrobble::Data::fromJson(json, stash));
	}

	Json Scrobble::toJson() const {
		return Json::object{
			{ "localID", (std::string)_localID },
			{ "scrobbler", (std::string)_scrobbler->name() },
			{ "startTime", (std::string)_startTime.toISOString() },
			{ "trackURI", _trackURI.empty() ? Json() : Json((std::string)_trackURI) },
			{ "musicBrainzID", _musicBrainzID.empty() ? Json() : Json((std::string)_musicBrainzID) },
			{ "trackName", (std::string)_trackName },
			{ "artistName", (std::string)_artistName },
			{ "albumName", _albumName.empty() ? Json() : Json((std::string)_albumName) },
			{ "albumArtistName", _albumArtistName.empty() ? Json() : Json((std::string)_albumArtistName) },
			{ "duration", _duration.hasValue() ? Json(_duration.value()) : Json() },
			{ "trackNumber", _trackNumber.hasValue() ? Json((double)_trackNumber.value()) : Json() },
			{ "chosenByUser", _chosenByUser.hasValue() ? Json(_chosenByUser.value()) : Json() },
			{ "historyItemStartTime", _historyItemStartTime.hasValue() ? Json((std::string)_historyItemStartTime->toISOString()) : Json() },
			{ "uploaded", _uploaded },
			{ "ignoredReason", _ignoredReason ? _ignoredReason->toJson() : Json() }
		};
	}
}
