//
//  MusicBrainzError.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/27/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class MusicBrainzError: public BasicError<> {
	public:
		enum class Code {
			REQUEST_FAILED,
			BAD_DATA
		};
		
		static String Code_toString(Code code);
		
		MusicBrainzError(Code code, String message);
		
		Code getCode() const;
		
		virtual String getMessage() const override;
		virtual String toString() const override;
		virtual const char* what() const noexcept override;
		
	private:
		Code code;
		String message;
	};
}
