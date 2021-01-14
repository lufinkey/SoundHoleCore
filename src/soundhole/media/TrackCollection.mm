//
//  TrackCollection.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 7/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "TrackCollection.hpp"
#ifdef __OBJC__
#import <Foundation/Foundation.h>

namespace sh {
	class TrackCollectionObjcSubscriber: public TrackCollection::AutoDeletedSubscriber {
	public:
		TrackCollectionObjcSubscriber(id<SHTrackCollectionSubscriber> subscriber);
		
		virtual void onTrackCollectionMutations($<TrackCollection> collection, const TrackCollection::ItemsListChange& change) override;
		
		__weak id<SHTrackCollectionSubscriber> weakSubscriber;
	};
	
	TrackCollectionObjcSubscriber::TrackCollectionObjcSubscriber(id<SHTrackCollectionSubscriber> subscriber)
	: weakSubscriber(subscriber) {
		//
	}
	
	void TrackCollectionObjcSubscriber::onTrackCollectionMutations($<TrackCollection> collection, const TrackCollection::ItemsListChange& change) {
		__strong id<SHTrackCollectionSubscriber> subscriber = weakSubscriber;
		if(subscriber == nil) {
			collection->unsubscribe(this);
			return;
		}
		if([subscriber respondsToSelector:@selector(trackCollection:didMutate:)]) {
			[subscriber trackCollection:collection didMutate:change];
		}
	}
	
	
	void TrackCollection::subscribe(id<SHTrackCollectionSubscriber> subscriber) {
		FGL_ASSERT(subscriber != nil, "subscriber cannot be nil");
		subscribe(new TrackCollectionObjcSubscriber(subscriber));
	}
	
	void TrackCollection::unsubscribe(id<SHTrackCollectionSubscriber> subscriber) {
		FGL_ASSERT(subscriber != nil, "subscriber cannot be nil");
		auto subscribers = this->subscribers();
		auto it = subscribers.findLastWhere([=](auto& cmpSubscriber) {
			auto cppSubscriber = dynamic_cast<TrackCollectionObjcSubscriber*>(cmpSubscriber);
			if(cppSubscriber == nullptr) {
				return false;
			}
			__strong id<SHTrackCollectionSubscriber> objSubscriber = cppSubscriber->weakSubscriber;
			if(subscriber == objSubscriber) {
				return true;
			}
			return false;
		});
		if(it != subscribers.end()) {
			unsubscribe(*it);
		}
	}
}

#endif
