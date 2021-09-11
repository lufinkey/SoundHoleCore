//
//  OmniTrack.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/7/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "OmniTrack.hpp"
#include "MediaProvider.hpp"

namespace sh {
	OmniTrack::Data OmniTrack::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		return Data{
			Track::Data::fromJson(json, stash),
			.musicBrainzID = json["musicBrainzID"].string_value(),
			.linkedTracks = ArrayList<$<Track>>(json["linkedTracks"].array_items(), [&](auto& json) {
				auto mediaItem = stash->parseMediaItem(json);
				if(!mediaItem) {
					throw std::invalid_argument("Invalid json for OmniTrack: elements of linkedTracks cannot be null");
				}
				auto track = std::dynamic_pointer_cast<Track>(mediaItem);
				if(!track) {
					throw std::invalid_argument("Invalid json for OmniTrack: parsed "+mediaItem->type()+" instead of expected type track in linkedTracks");
				}
				if(std::dynamic_pointer_cast<OmniTrack>(track)) {
					throw std::invalid_argument("Invalid json for OmniTrack: linked track of OmniTrack cannot be an OmniTrack");
				}
				return track;
			})
		};
	}

	$<OmniTrack> OmniTrack::new$(MediaProvider* provider, const Data& data) {
		return fgl::new$<OmniTrack>(provider, data);
	}

	OmniTrack::OmniTrack(MediaProvider* provider, const Data& data)
	: Track(provider, data), _musicBrainzID(data.musicBrainzID), _linkedTracks(data.linkedTracks) {
		for(auto& track : _linkedTracks) {
			if(std::dynamic_pointer_cast<OmniTrack>(track)) {
				throw std::logic_error("Linked track of OmniTrack cannot be an omniTrack");
			}
		}
	}

	bool OmniTrack::isPlayable() const {
		// check if this track or any linked tracks are playable
		auto playable = this->playable();
		size_t missingPlayableCount = 0;
		if(playable.hasValue()) {
			if(playable.value()) {
				return true;
			}
		} else if(this->needsData()) {
			missingPlayableCount += 1;
		}
		for(auto& track : _linkedTracks) {
			playable = track->playable();
			if(playable.hasValue()) {
				if(playable.value()) {
					return true;
				}
			} else if(track->needsData()) {
				missingPlayableCount += 1;
			}
		}
		if(missingPlayableCount > 0) {
			return true;
		}
		return false;
	}

	const String& OmniTrack::musicBrainzID() const {
		return _musicBrainzID;
	}

	const ArrayList<$<Track>>& OmniTrack::linkedTracks() {
		return _linkedTracks;
	}

	const ArrayList<$<const Track>>& OmniTrack::linkedTracks() const {
		return *((const ArrayList<$<const Track>>*)(&_linkedTracks));
	}

	Promise<void> OmniTrack::fetchData() {
		// TODO fetch omni track data (get music brainz ID, link other tracks, etc)
		/*return Promise<void>::all(_linkedTracks.map([](auto& track) {
			return track->fetchDataIfNeeded();
		})).then([]() {
			
		});*/
		return Track::fetchData();
	}

	void OmniTrack::applyData(const Data& data) {
		Track::applyData(data);
		if(!data.musicBrainzID.empty()) {
			_musicBrainzID = data.musicBrainzID;
		}
		if(!data.linkedTracks.empty()) {
			// combine track arrays
			auto newTracks = _linkedTracks;
			newTracks.reserve(std::max(_linkedTracks.size(), data.linkedTracks.size()));
			// go through data.linkedTracks and apply data to any existing tracks where needed
			for(auto& track : data.linkedTracks) {
				auto existingTrack = _linkedTracks.firstWhere([&](auto& cmpTrack) {
					return (track->mediaProvider() == cmpTrack->mediaProvider() && track->uri() == cmpTrack->uri()
						&& (!track->uri().empty() || track->name() == cmpTrack->name()));
				}, nullptr);
				if(existingTrack) {
					// check if we need data or new track isn't partial
					if(existingTrack->needsData() || !track->needsData()) {
						existingTrack->applyData(track->toData());
					}
				} else {
					newTracks.pushBack(track);
				}
			}
			// apply new tracks
			_linkedTracks = newTracks;
		}
		// if a linked track matches this current track, apply the data there too
		if(!data.uri.empty()) {
			auto matchedTrack = linkedTrackWhere([&](auto& cmpTrack) {
				return cmpTrack->uri() == data.uri && cmpTrack->mediaProvider()->name() == this->mediaProvider()->name();
			});
			if(matchedTrack) {
				matchedTrack->applyData(Track::Data(data));
			}
		}
	}

	OmniTrack::Data OmniTrack::toData() const {
		return Data{
			Track::toData(),
			.musicBrainzID = _musicBrainzID,
			.linkedTracks = _linkedTracks
		};
	}

	Json OmniTrack::toJson() const {
		Json::object json = Track::toJson().object_items();
		json["musicBrainzID"] = _musicBrainzID;
		json["linkedTracks"] = _linkedTracks.map([](auto& track) {
			return track->toJson();
		});
		return json;
	}
}
