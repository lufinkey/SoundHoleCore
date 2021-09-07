//
//  LastFMError.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class LastFMError: public BasicError<> {
	public:
		enum class Code {
			REQUEST_FAILED,
			BAD_DATA
		};
		static String Code_toString(Code code);
		
		LastFMError(Code code, String message, Map<String,Any> details = {});
		
		Code getCode() const;
		const Map<String,Any>& getDetails() const;
		Any getDetail(const String& key) const override;
		
		virtual String getMessage() const override;
		virtual String toString() const override;
		virtual const char* what() const noexcept override;
		
	private:
		Code code;
		String message;
		Map<String,Any> details;
	};
}
