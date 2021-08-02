//
//  JSError.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 7/16/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class JSError: public BasicError<> {
	public:
		JSError(napi_env, napi_value);
		JSError(JSError&&);
		JSError(const JSError&);
		~JSError();
		
		napi_ref getJSRef() const;
		napi_value getJSValue(napi_env) const;
		
		virtual String getMessage() const override;
		virtual String toString() const override;
		virtual Any getDetail(const String& key) const override;
		
		const String& getJSStack() const;
		
	private:
		napi_ref jsRef;
		String code;
		String stack;
		String message;
	};
}
