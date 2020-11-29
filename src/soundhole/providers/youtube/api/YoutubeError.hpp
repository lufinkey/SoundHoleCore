//
//  YoutubeError.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class YoutubeError: public BasicError<> {
	public:
		enum class Code {
			NOT_INITIALIZED,
			REQUEST_NOT_SENT,
			REQUEST_FAILED,
			SESSION_EXPIRED,
			BAD_DATA,
			NOT_FOUND,
			
			OAUTH_REQUEST_FAILED
		};
		
		static String Code_toString(Code code);
		
		YoutubeError(Code code, String message, std::map<String,Any> details = {});
		
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
