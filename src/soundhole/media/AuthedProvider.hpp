//
//  AuthenticatedProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/3/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class AuthedProvider {
	public:
		virtual Promise<bool> login() = 0;
		virtual void logout() = 0;
		virtual bool isLoggedIn() const = 0;
		virtual Promise<ArrayList<String>> getCurrentUserIds() = 0;
	};
}
