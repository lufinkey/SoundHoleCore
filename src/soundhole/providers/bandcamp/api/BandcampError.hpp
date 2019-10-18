//
//  BandcampError.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/16/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class BandcampError: public Error {
	public:
		enum class Code {
			NOT_INITIALIZED,
			REQUEST_FAILED,
			BAD_RESPONSE
		};
		
		static String Code_toString(Code code);
		
		BandcampError(Code code, String message);
		
		Code getCode() const;
		
		virtual String getMessage() const override;
		virtual String toString() const override;
		
	private:
		Code code;
		String message;
	};
}
