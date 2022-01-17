//
//  PlaybackHistoryItem.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/22/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "PlaybackHistoryItem.hpp"

namespace sh {
	PlaybackHistoryItem::Data PlaybackHistoryItem::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto mediaItem = stash->parseMediaItem(json["track"]);
		if(!mediaItem) {
			throw std::invalid_argument("Invalid json for PlaybackHistoryItem: 'track' property is missing");
		}
		auto track = std::dynamic_pointer_cast<Track>(mediaItem);
		if(!track) {
			throw std::invalid_argument("Invalid json for PlaybackHistoryItem: 'track' property cannot have type '"+mediaItem->type()+"'");
		}
		auto startTime = json["startTime"];
		if(startTime.is_null()) {
			throw std::invalid_argument("Invalid json for PlaybackHistoryItem: 'startTime' property is missing");
		}
		auto duration = json["duration"];
		auto chosenByUser = json["chosenByUser"];
		return PlaybackHistoryItem::Data{
			.track = track,
			.startTime =
				Date::maybeFromISOString(startTime.string_value())
					.valueOrThrow(std::invalid_argument("Invalid date string \""+startTime.string_value()+"\" for PlaybackHistoryItem property 'startTime'")),
			.contextURI = json["contextURI"].string_value(),
			.duration = duration.is_number() ? maybe(duration.number_value()) : std::nullopt,
			.chosenByUser = chosenByUser.is_bool() ? chosenByUser.bool_value()
				: chosenByUser.is_number() ?
					(chosenByUser.number_value() == 0) ? false
					: (chosenByUser.number_value() == 1) ? true
					: true
				: true,
			.visibility = PlaybackHistoryItem::Visibility_fromString(json["visibility"].string_value())
		};
	}

	PlaybackHistoryItem::Visibility PlaybackHistoryItem::Visibility_fromString(const String& str) {
		if(str == "UNSAVED") {
			return Visibility::UNSAVED;
		} else if(str == "SCROBBLES") {
			return Visibility::SCROBBLES;
		} else if(str == "HISTORY") {
			return Visibility::HISTORY;
		}
		throw std::invalid_argument("invalid value for PlaybackHistoryItem::Visibility "+str);
	}

	String PlaybackHistoryItem::Visibility_toString(Visibility visibility) {
		switch(visibility) {
			case Visibility::UNSAVED:
				return "UNSAVED";
				
			case Visibility::SCROBBLES:
				return "SCROBBLES";
				
			case Visibility::HISTORY:
				return "HISTORY";
		}
		throw std::invalid_argument("invalid PlaybackHistoryItem::Visibility value "+std::to_string((long)visibility));
	}

	$<PlaybackHistoryItem> PlaybackHistoryItem::new$(Data data) {
		return fgl::new$<PlaybackHistoryItem>(data);
	}

	$<PlaybackHistoryItem> PlaybackHistoryItem::fromJson(const Json& json, MediaProviderStash* stash) {
		return fgl::new$<PlaybackHistoryItem>(Data::fromJson(json, stash));
	}

	PlaybackHistoryItem::PlaybackHistoryItem(Data data)
		: _track(data.track),
		_startTime(data.startTime),
		_contextURI(data.contextURI),
		_duration(data.duration),
		_chosenByUser(data.chosenByUser),
		_visibility(data.visibility) {
		//
	}

	PlaybackHistoryItem::~PlaybackHistoryItem() {
		//
	}

	$<Track> PlaybackHistoryItem::track() {
		return _track;
	}

	$<const Track> PlaybackHistoryItem::track() const {
		return std::static_pointer_cast<const Track>(_track);
	}

	const Date& PlaybackHistoryItem::startTime() const {
		return _startTime;
	}

	const String& PlaybackHistoryItem::contextURI() const {
		return _contextURI;
	}

	const Optional<double>& PlaybackHistoryItem::duration() const {
		return _duration;
	}

	bool PlaybackHistoryItem::chosenByUser() const {
		return _chosenByUser;
	}

	PlaybackHistoryItem::Visibility PlaybackHistoryItem::visibility() const {
		return _visibility;
	}

	void PlaybackHistoryItem::increaseDuration(double amount) {
		_duration = _duration.valueOr(0) + amount;
	}

	void PlaybackHistoryItem::setDuration(Optional<double> duration) {
		_duration = duration;
	}

	bool PlaybackHistoryItem::matchesItem(const PlayerItem& cmpItem) const {
		auto cmpContext = cmpItem.isCollectionItem() ? cmpItem.asCollectionItem()->context().lock() : nullptr;
		auto cmpContextURI = cmpContext ? cmpContext->uri() : String();
		auto matchingTrack = cmpItem.linkedTrackWhere([&](auto& cmpTrack) {
			return _track->uri() == cmpTrack->uri();
		});
		if(matchingTrack != nullptr && _contextURI == cmpContextURI) {
			return true;
		}
		return false;
	}

	bool PlaybackHistoryItem::matches(const PlaybackHistoryItem* historyItem) const {
		return _track->uri() == historyItem->track()->uri()
			&& _startTime == historyItem->startTime();
	}

	PlaybackHistoryItem::Data PlaybackHistoryItem::toData() const {
		return Data{
			.track = _track,
			.startTime = _startTime,
			.contextURI = _contextURI,
			.duration = _duration,
			.chosenByUser = _chosenByUser,
			.visibility = _visibility,
		};
	}

	Json PlaybackHistoryItem::toJson() const {
		return Json::object{
			{ "type", "playbackHistoryItem" },
			{ "track", _track->toJson() },
			{ "startTime", (std::string)_startTime.toISOString() },
			{ "contextURI", (std::string)_contextURI },
			{ "duration", _duration ? _duration.value() : Json() },
			{ "chosenByUser", _chosenByUser },
			{ "visibility", (std::string)Visibility_toString(_visibility) }
		};
	}
}
