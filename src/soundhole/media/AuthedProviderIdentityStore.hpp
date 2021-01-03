//
//  AuthedProviderIdentityStore.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/3/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "AuthedProvider.hpp"
#include "NamedProvider.hpp"

namespace sh {
	template<typename IdentityType>
	class AuthedProviderIdentityStore: public virtual AuthedProvider {
	public:
		AuthedProviderIdentityStore();
		
		void setIdentityNeedsRefresh();
		Promise<Optional<IdentityType>> getIdentity();
		
	protected:
		virtual Promise<Optional<IdentityType>> fetchIdentity() = 0;
		virtual String getIdentityFilePath() const = 0;
		void storeIdentity(Optional<IdentityType> identity);
		
	private:
		Optional<IdentityType> _identity;
		Optional<Promise<Optional<IdentityType>>> _identityPromise;
		bool _identityNeedsRefresh;
	};
}

#include "AuthedProviderIdentityStore.impl.hpp"
