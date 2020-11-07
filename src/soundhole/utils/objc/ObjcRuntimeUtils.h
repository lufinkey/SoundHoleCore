//
//  ObjcRuntimeUtils.h
//  SoundHoleCore
//
//  Created by Luis Finke on 11/7/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <objc/message.h>

namespace sh {
	template<typename T>
	inline T performObjcCppSelector(id target, SEL selector) {
		return ((T(*)(id,SEL))objc_msgSend_stret)(target, selector);
	}

	template<typename T, typename Arg>
	inline T performObjcCppSelector(id target, SEL selector, Arg arg) {
		return ((T(*)(id,SEL,Arg))objc_msgSend_stret)(target, selector, arg);
	}
}
