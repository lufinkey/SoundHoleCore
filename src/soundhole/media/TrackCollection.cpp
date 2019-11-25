//
//  TrackCollection.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "TrackCollection.hpp"

namespace sh {
	TrackCollectionItem::TrackCollectionItem($<TrackCollection> context, Data data)
	: _context(context), _track(data.track) {
		//
	}

	$<Track> TrackCollectionItem::track() {
		return _track;
	}

	$<const Track> TrackCollectionItem::track() const {
		return std::static_pointer_cast<const Track>(_track);
	}

	w$<TrackCollection> TrackCollectionItem::context() {
		return _context;
	}

	w$<const TrackCollection> TrackCollectionItem::context() const {
		return _context;
	}
	
	Optional<size_t> TrackCollectionItem::indexInContext() const {
		auto context = _context.lock();
		if(!context) {
			// TODO maybe fatal error?
			return std::nullopt;
		}
		return context->indexOfItem(this);
	}
}
