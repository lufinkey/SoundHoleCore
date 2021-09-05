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
				: true
		};
	}

	$<PlaybackHistoryItem> PlaybackHistoryItem::new$(const Data& data) {
		return fgl::new$<PlaybackHistoryItem>(data);
	}

	$<PlaybackHistoryItem> PlaybackHistoryItem::fromJson(const Json& json, MediaProviderStash* stash) {
		return fgl::new$<PlaybackHistoryItem>(Data::fromJson(json, stash));
	}

	PlaybackHistoryItem::PlaybackHistoryItem(const Data& data)
		: _track(data.track),
		_startTime(data.startTime),
		_contextURI(data.contextURI),
		_duration(data.duration),
		_chosenByUser(data.chosenByUser) {
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

	void PlaybackHistoryItem::increaseDuration(double amount) {
		_duration = _duration.valueOr(0) + amount;
	}

	void PlaybackHistoryItem::setDuration(Optional<double> duration) {
		_duration = duration;
	}

	bool PlaybackHistoryItem::matchesItem(const PlayerItem& cmpItem) const {
		auto cmpContext = cmpItem.isCollectionItem() ? cmpItem.asCollectionItem()->context().lock() : nullptr;
		auto cmpContextURI = cmpContext ? cmpContext->uri() : String();
		auto cmpTrackURI = cmpItem.track()->uri();
		if(_track->uri() == cmpTrackURI && _contextURI == cmpContextURI) {
			return true;
		}
		return false;
	}

	PlaybackHistoryItem::Data PlaybackHistoryItem::toData() const {
		return Data{
			.track = _track,
			.startTime = _startTime,
			.contextURI = _contextURI,
			.duration = _duration,
			.chosenByUser = _chosenByUser
		};
	}

	Json PlaybackHistoryItem::toJson() const {
		return Json::object{
			{ "type", "playbackHistoryItem" },
			{ "track", _track->toJson() },
			{ "startTime", (std::string)_startTime.toISOString() },
			{ "contextURI", (std::string)_contextURI },
			{ "duration", _duration ? _duration.value() : Json() },
			{ "chosenByUser", _chosenByUser }
		};
	}
}
