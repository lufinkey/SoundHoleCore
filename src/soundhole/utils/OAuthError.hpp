//
//  OAuthError.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class OAuthError: public BasicError<> {
	public:
		enum class Code {
			REQUEST_NOT_SENT,
			REQUEST_FAILED,
			BAD_DATA,
			INVALID_REQUEST,
			INVALID_CLIENT,
			INVALID_GRANT,
			INVALID_SCOPE,
			UNAUTHORIZED_CLIENT,
			UNSUPPORTED_GRANT_TYPE,
			SESSION_EXPIRED,
			VERIFIER_MISMATCH
		};
		
		static String Code_toString(Code code);
		static Optional<Code> Code_fromOAuthError(String error);
		static String Code_getDescription(Code code);
		
		OAuthError(Code code, String message, std::map<String,Any> details = {});
		
		Code getCode() const;
		const std::map<String,Any>& getDetails() const;
		Any getDetail(const String& key) const override;
		
		virtual String getMessage() const override;
		virtual String toString() const override;
		virtual const char* what() const noexcept override;
		
	private:
		Code code;
		String message;
		std::map<String,Any> details;
	};
}
