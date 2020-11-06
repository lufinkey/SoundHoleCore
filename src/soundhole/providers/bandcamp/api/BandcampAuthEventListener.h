//
//  BandcampAuthEventListener.h
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class BandcampAuth;
	
	class BandcampAuthEventListener {
	public:
		virtual ~BandcampAuthEventListener() {}
		
		virtual void onBandcampAuthSessionResume(BandcampAuth* auth) {}
		virtual void onBandcampAuthSessionStart(BandcampAuth* auth) {}
		virtual void onBandcampAuthSessionUpdate(BandcampAuth* auth) {}
		virtual void onBandcampAuthSessionEnd(BandcampAuth* auth) {}
	};
}
