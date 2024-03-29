//
//  SoundHoleError.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/20/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class SoundHoleError: public BasicError<> {
	public:
		enum class Code {
			REQUEST_NOT_SENT,
			REQUEST_FAILED,
			STREAM_UNAVAILABLE,
			PARSE_FAILED
		};
		static String Code_toString(Code code);
		
		SoundHoleError(Code code, String reason);
		
		Code getCode() const;
		String getCodeName() const;
		virtual String getMessage() const override;
		virtual String toString() const override;
		virtual const char* what() const noexcept override;
		
	private:
		Code code;
		String message;
	};
}
