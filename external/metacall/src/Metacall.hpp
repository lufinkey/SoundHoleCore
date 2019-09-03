//
//  Metacall.hpp
//  metacall-core
//
//  Created by Luis Finke on 8/28/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <any>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace mc {
	class Metacall {
	public:
		enum class Lang {
			MOCK,
			NODE,
			PY
		};
		
		Metacall(const Metacall&) = delete;
		Metacall& operator=(const Metacall&) = delete;
		
		Metacall();
		~Metacall();
		
		void loadFromFiles(Lang lang, const std::vector<std::string>& files);
		void loadFromFile(Lang lang, const std::string& file);
		void loadFromString(Lang lang, const std::string& buffer);
		void loadFromMemory(Lang lang, const char* buffer, size_t buffer_size);
		
		std::any call(const std::string& func, const std::vector<std::any>& args);
		
		static void* toMetacallValue(std::any any);
		static std::any fromMetacallValue(void* value);
		static void destroyMetacallValue(void* value);
		
	private:
		std::map<Lang,std::list<void*>> handles;
	};
}
