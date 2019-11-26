//
//  UserAccount.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/25/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "UserAccount.hpp"

namespace sh {
	$<UserAccount> UserAccount::new$(MediaProvider* provider, Data data) {
		return fgl::new$<UserAccount>(provider, data);
	}

	bool UserAccount::needsData() const {
		// TODO implement needsData
		return false;
	}

	Promise<void> UserAccount::fetchMissingData() {
		// TODO implement fetchMissingData
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}
}
