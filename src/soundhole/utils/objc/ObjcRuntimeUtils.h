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
	inline T performObjcSelector(id target, SEL selector) {
		return ((T(*)(id,SEL))objc_msgSend)(target, selector);
	}

	template<typename T, typename Arg>
	inline T performObjcSelector(id target, SEL selector, Arg arg) {
		return ((T(*)(id,SEL,Arg))objc_msgSend)(target, selector, arg);
	}


	template<typename T>
	inline T performObjcStretSelector(id target, SEL selector) {
		#ifdef __aarch64__
			return performObjcSelector<T>(target,selector);
		#else
			return ((T(*)(id,SEL))objc_msgSend_stret)(target, selector);
		#endif
	}

	template<typename T, typename Arg>
	inline T performObjcStretSelector(id target, SEL selector, Arg arg) {
		#ifdef __aarch64__
			return performObjcSelector<T,Arg>(target,selector,arg);
		#else
			return ((T(*)(id,SEL,Arg))objc_msgSend_stret)(target, selector, arg);
		#endif
	}


	template<typename T>
	inline T performObjcFpretSelector(id target, SEL selector) {
		#ifdef __aarch64__
			return performObjcSelector<T,Arg>(target,selector);
		#else
			return ((T(*)(id,SEL))objc_msgSend_fpret)(target, selector);
		#endif
	}

	template<typename T, typename Arg>
	inline T performObjcFpretSelector(id target, SEL selector, Arg arg) {
		#ifdef __aarch64__
			return performObjcSelector<T,Arg>(target,selector,arg);
		#else
			return ((T(*)(id,SEL,Arg))objc_msgSend_fpret)(target, selector, arg);
		#endif
	}
}
