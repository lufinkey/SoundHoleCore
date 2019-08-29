//
//  Metacall.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/28/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <map>
#include <soundhole/external.hpp>

namespace sh {
	class Metacall {
	public:
		enum class Lang {
			NODE,
			PY
		};
		
		Metacall(const Metacall&) = delete;
		Metacall& operator=(const Metacall&) = delete;
		
		Metacall();
		~Metacall();
		
		void loadFromFiles(Lang lang, const ArrayList<String>& files);
		void loadFromFile(Lang lang, const String& file);
		void loadFromString(Lang lang, const String& buffer);
		void loadFromMemory(Lang lang, const char* buffer, size_t buffer_size);
		
		Any call(const String& func, const ArrayList<Any>& args);
		
		static void* toMetacallValue(Any any);
		static Any fromMetacallValue(void* value);
		static void destroyMetacallValue(void* value);
		
	private:
		std::map<Lang,ArrayList<void*>> handles;
	};
}
