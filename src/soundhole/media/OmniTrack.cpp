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
	: Track(provider, data), _linkedTracks(data.linkedTracks) {
		for(auto& track : _linkedTracks) {
			if(track.as<OmniTrack>() != nullptr) {
				throw std::logic_error("Linked track of OmniTrack cannot be an OmniTrack");
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

	const ArrayList<$<Track>>& OmniTrack::linkedTracks() {
		return _linkedTracks;
	}

	const ArrayList<$<const Track>>& OmniTrack::linkedTracks() const {
		return *((const ArrayList<$<const Track>>*)(&_linkedTracks));
	}

	Promise<void> OmniTrack::fetchData() {
		// TODO fetch omni track data (link other tracks, etc)
		/*return Promise<void>::all(_linkedTracks.map([](auto& track) {
			return track->fetchDataIfNeeded();
		})).then([]() {
			
		});*/
		return Track::fetchData();
	}

	void OmniTrack::applyData(const Data& data) {
		Track::applyData(data);
		for(auto& track : data.linkedTracks) {
			if(track.as<OmniTrack>() != nullptr) {
				throw std::invalid_argument("Linked track for OmniTrack cannot also be an OmniTrack");
			}
		}
		if(!data.linkedTracks.empty()) {
			// combine track arrays
			auto newTracks = _linkedTracks;
			newTracks.reserve(std::max(_linkedTracks.size(), data.linkedTracks.size()));
			// go through data.linkedTracks and apply data to any existing tracks where needed
			for(auto& newTrack : data.linkedTracks) {
				auto existingTrackIt = _linkedTracks.findWhere([&](auto& cmpTrack) {
					return (newTrack->mediaProvider()->name() == cmpTrack->mediaProvider()->name()
						&& ((cmpTrack->uri().empty() && newTrack->name() == cmpTrack->name())
							|| (!cmpTrack->uri().empty() && newTrack->uri() == cmpTrack->uri())));
				});
				if(existingTrackIt != _linkedTracks.end()) {
					auto& existingTrack = *existingTrackIt;
					// if new track encapsulates the existing track
					if(newTrack->isSameClassAs(existingTrack)) {
						// apply data only if we need data or new track isn't partial
						if(existingTrack->needsData() || !newTrack->needsData()) {
							existingTrack->applyDataFrom(newTrack);
						}
					} else {
						// new track is a different type, so just use new track
						existingTrack = newTrack;
					}
				} else {
					newTracks.pushBack(newTrack);
				}
			}
			// apply new tracks
			_linkedTracks = newTracks;
		}
		// if a linked track matches this current track, apply the data there too
		if(!data.uri.empty()) {
			auto matchedTrack = _linkedTracks.firstWhere([&](auto& cmpTrack) {
				return cmpTrack->mediaProvider()->name() == this->mediaProvider()->name() && cmpTrack->uri() == data.uri;
			}, nullptr);
			if(matchedTrack) {
				matchedTrack->applyDataFrom(_$(shared_from_this()).forceAs<Track>());
			}
		}
	}

	void OmniTrack::applyDataFrom($<const Track> track) {
		if(auto omniTrack = track.as<const OmniTrack>()) {
			applyData(omniTrack->toData());
		} else {
			Track::applyDataFrom(track);
		}
	}

	bool OmniTrack::isSameClassAs($<const Track> track) const {
		if(track.as<const OmniTrack>() == nullptr) {
			return false;
		}
		return true;
	}

	OmniTrack::Data OmniTrack::toData() const {
		return Data{
			Track::toData(),
			.linkedTracks = _linkedTracks
		};
	}

	Json OmniTrack::toJson() const {
		Json::object json = Track::toJson().object_items();
		json["linkedTracks"] = _linkedTracks.map([](auto& track) {
			return track->toJson();
		});
		return json;
	}
}
