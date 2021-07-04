//
//  AuthedProviderIdentityStore.impl.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/3/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	template<typename IdentityType>
	AuthedProviderIdentityStore<IdentityType>::AuthedProviderIdentityStore()
	: _identityNeedsRefresh(true) {
		//
	}

	template<typename IdentityType>
	void AuthedProviderIdentityStore<IdentityType>::setIdentityNeedsRefresh() {
		_identityNeedsRefresh = true;
	}


	template<typename IdentityType>
	Promise<Optional<IdentityType>> AuthedProviderIdentityStore<IdentityType>::getIdentity() {
		using IdentityPromise = Promise<Optional<IdentityType>>;
		if(!isLoggedIn()) {
			storeIdentity(std::nullopt);
			return IdentityPromise::resolve(std::nullopt);
		}
		if(_identityPromise) {
			return _identityPromise.value();
		}
		if(_identity && !_identityNeedsRefresh) {
			return IdentityPromise::resolve(_identity);
		}
		auto promise = fetchIdentity().map([=](Optional<IdentityType> identity) -> Optional<IdentityType> {
			this->_identityPromise = std::nullopt;
			if(!this->isLoggedIn()) {
				this->storeIdentity(std::nullopt);
				return std::nullopt;
			}
			this->storeIdentity(identity);
			return identity;
		}).except([=](std::exception_ptr error) -> Optional<IdentityType> {
			this->_identityPromise = std::nullopt;
			// if identity is loaded, return it
			if(this->_identity) {
				return this->_identity.value();
			}
			// try to load identity from filesystem
			auto identityFilePath = getIdentityFilePath();
			if(!identityFilePath.empty()) {
				try {
					String identityString = fs::readFile(identityFilePath);
					std::string jsonError;
					Json identityJson = Json::parse(identityString, jsonError);
					if(!jsonError.empty()) {
						throw std::runtime_error("failed to parse identity json: "+jsonError);
					} else if(identityJson.is_null()) {
						throw std::runtime_error("identity json is empty");
					}
					auto identity = IdentityType::fromJson(identityJson);
					_identity = identity;
					_identityNeedsRefresh = true;
					return identity;
				} catch(...) {
					// ignore error
				}
			}
			// rethrow error
			std::rethrow_exception(error);
		});
		_identityPromise = promise;
		if(_identityPromise && _identityPromise->isComplete()) {
			_identityPromise = std::nullopt;
		}
		return promise;
	}


	template<typename IdentityType>
	void AuthedProviderIdentityStore<IdentityType>::storeIdentity(Optional<IdentityType> identity) {
		auto identityFilePath = getIdentityFilePath();
		if(identity) {
			_identity = identity;
			_identityNeedsRefresh = false;
			if(!identityFilePath.empty()) {
				fs::writeFile(identityFilePath, identity->toJson().dump());
			}
		} else {
			_identity = std::nullopt;
			_identityPromise = std::nullopt;
			_identityNeedsRefresh = true;
			if(!identityFilePath.empty() && fs::exists(identityFilePath)) {
				fs::remove(identityFilePath);
			}
		}
	}
}
