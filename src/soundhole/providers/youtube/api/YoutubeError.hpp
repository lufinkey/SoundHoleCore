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
	class YoutubeError: public Error {
	public:
		enum class Code {
			NOT_INITIALIZED,
			REQUEST_FAILED,
			BAD_DATA,
			NOT_FOUND
		};
		
		static String Code_toString(Code code);
		
		YoutubeError(Code code, String message);
		
		Code getCode() const;
		
		virtual String getMessage() const override;
		virtual String toString() const override;
		
	private:
		Code code;
		String message;
	};
}
