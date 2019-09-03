//
//  Metacall.cpp
//  metacall-core
//
//  Created by Luis Finke on 8/28/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Metacall.hpp"
#include <stdexcept>
#include <typeinfo>
#include <metacall/metacall.h>

namespace mc {
	static bool Metacall_initialized = false;
	
	void Metacall_init() {
		if(Metacall_initialized) {
			return;
		}
		struct metacall_log_stdio_type log = { stdout };
		metacall_log( METACALL_LOG_STDIO, (void*)&log);
		int result = metacall_initialize();
		if(result != 0) {
			throw std::runtime_error("Error initializing metacall ("+std::to_string(result)+")");
		}
		Metacall_initialized = true;
	}
	
	std::string MetacallLang_getTag(Metacall::Lang lang) {
		switch(lang) {
			case Metacall::Lang::MOCK:
				return "mock";
			case Metacall::Lang::NODE:
				return "node";
			case Metacall::Lang::PY:
				return "py";
			default:
				// invalid Lang enum
				assert(false);
		}
	}
	
	
	
	Metacall::Metacall() {
		Metacall_init();
	}
	
	Metacall::~Metacall() {
		for(auto& pair : handles) {
			for(auto handle : pair.second) {
				metacall_clear(handle);
			}
		}
	}
	
	
	
	void Metacall::loadFromFiles(Lang lang, const std::vector<std::string>& files) {
		auto tag = MetacallLang_getTag(lang);
		auto files_ptr = std::make_unique<const char*[]>(files.size());
		for(size_t i=0; i<files.size(); i++) {
			files_ptr[i] = files[i].data();
		}
		void* handle = nullptr;
		int result = metacall_load_from_file(tag.data(), files_ptr.get(), files.size(), &handle);
		files_ptr.reset();
		if(result != 0) {
			if(handle != nullptr) {
				metacall_clear(handle);
			}
			throw std::runtime_error("Error loading files ("+std::to_string(result)+")");
		}
		if(handles.find(lang) != handles.end()) {
			handles[lang].push_back(handle);
		} else {
			handles[lang] = { handle };
		}
	}
	
	void Metacall::loadFromFile(Lang lang, const std::string& file) {
		loadFromFiles(lang, { file });
	}
	
	void Metacall::loadFromString(Lang lang, const std::string& buffer) {
		loadFromMemory(lang, buffer.data(), buffer.length());
	}
	
	void Metacall::loadFromMemory(Lang lang, const char* buffer, size_t buffer_size) {
		auto tag = MetacallLang_getTag(lang);
		void* handle = nullptr;
		int result = metacall_load_from_memory(tag.data(), buffer, buffer_size, &handle);
		if(result != 0) {
			if(handle != nullptr) {
				metacall_clear(handle);
			}
			throw std::runtime_error("Error loading files ("+std::to_string(result)+")");
		}
		if(handles.find(lang) != handles.end()) {
			handles[lang].push_back(handle);
		} else {
			handles[lang] = { handle };
		}
	}
	
	
	
	
	template<typename ArrayType>
	void* Metacall_convertArrayToValue(std::any any) {
		auto list = std::any_cast<ArrayType>(any);
		auto list_ptr = std::make_unique<void*[]>(list.size());
		size_t i=0;
		for(auto& item : list) {
			list_ptr[i] = Metacall::toMetacallValue(item);
			i++;
		}
		void* list_value = metacall_value_create_array((const void**)list_ptr.get(), list.size());
		for(size_t i=0; i<list.size(); i++) {
			Metacall::destroyMetacallValue(list_ptr[i]);
		}
		list_ptr.reset();
		return list_value;
	}
	
	template<typename MapType>
	void* Metacall_convertMapToValue(std::any any) {
		auto map = std::any_cast<MapType>(any);
		auto map_ptr = std::make_unique<void**[]>(map.size());
		size_t i=0;
		for(auto& pair : map) {
			void** tuple = new void*[2];
			tuple[0] = Metacall::toMetacallValue(pair.first);
			tuple[1] = Metacall::toMetacallValue(pair.second);
			map_ptr[i] = tuple;
			i++;
		}
		void* map_value = metacall_value_create_map((const void**)map_ptr.get(), map.size());
		for(size_t i=0; i<map.size(); i++) {
			Metacall::destroyMetacallValue(map_ptr[i][0]);
			Metacall::destroyMetacallValue(map_ptr[i][1]);
		}
		map_ptr.reset();
		return map_value;
	}
	
	using Any = std::any;
	using String = std::string;
	template<typename T>
	using ArrayList = std::vector<T>;
	template<typename T>
	using LinkedList = std::list<T>;
	template<typename T, typename U>
	using Map = std::map<T,U>;
	
	void* Metacall::toMetacallValue(std::any any) {
		auto typeHash = any.type().hash_code();
		#define mcallCheckType(...) (typeHash == typeid(__VA_ARGS__).hash_code())
		
		// null
		if(!any.has_value()) {
			return metacall_value_create_null();
		}
		// containers
		else if(mcallCheckType(ArrayList<Any>)) {
			return Metacall_convertArrayToValue<ArrayList<Any>>(any);
		} else if(mcallCheckType(Map<String,Any>)) {
			return Metacall_convertMapToValue<Map<String,Any>>(any);
		} else if(mcallCheckType(LinkedList<Any>)) {
			return Metacall_convertArrayToValue<LinkedList<Any>>(any);
		}
		// basic types
		else if(mcallCheckType(String)) {
			auto str = std::any_cast<String>(any);
			return metacall_value_create_string(str.data(), str.length());
		} else if(mcallCheckType(int)) {
			return metacall_value_create_int(std::any_cast<int>(any));
		} else if(mcallCheckType(long)) {
			return metacall_value_create_long(std::any_cast<long>(any));
		} else if(mcallCheckType(bool)) {
			return metacall_value_create_bool(std::any_cast<bool>(any));
		} else if(mcallCheckType(void*)) {
			return std::any_cast<void*>(any);
		} else if(mcallCheckType(float)) {
			return metacall_value_create_float(std::any_cast<float>(any));
		} else if(mcallCheckType(double)) {
			return metacall_value_create_double(std::any_cast<double>(any));
		} else if(mcallCheckType(short)) {
			return metacall_value_create_short(std::any_cast<short>(any));
		} else if(mcallCheckType(char)) {
			return metacall_value_create_char(std::any_cast<char>(any));
		}
		// specialized ArrayList
		else if(mcallCheckType(ArrayList<String>)) {
			return Metacall_convertArrayToValue<ArrayList<String>>(any);
		} else if(mcallCheckType(ArrayList<int>)) {
			return Metacall_convertArrayToValue<ArrayList<int>>(any);
		} else if(mcallCheckType(ArrayList<long>)) {
			return Metacall_convertArrayToValue<ArrayList<long>>(any);
		} else if(mcallCheckType(ArrayList<void*>)) {
			return Metacall_convertArrayToValue<ArrayList<void*>>(any);
		} else if(mcallCheckType(ArrayList<float>)) {
			return Metacall_convertArrayToValue<ArrayList<float>>(any);
		} else if(mcallCheckType(ArrayList<double>)) {
			return Metacall_convertArrayToValue<ArrayList<double>>(any);
		} else if(mcallCheckType(ArrayList<short>)) {
			return Metacall_convertArrayToValue<ArrayList<short>>(any);
		} else if(mcallCheckType(ArrayList<char>)) {
			return Metacall_convertArrayToValue<ArrayList<char>>(any);
		} else if(mcallCheckType(ArrayList<ArrayList<Any>>)) {
			return Metacall_convertArrayToValue<ArrayList<ArrayList<Any>>>(any);
		} else if(mcallCheckType(ArrayList<Map<String,Any>>)) {
			return Metacall_convertArrayToValue<ArrayList<Map<String,Any>>>(any);
		}
		// specialized Map
		else if(mcallCheckType(Map<String,String>)) {
			return Metacall_convertMapToValue<Map<String,String>>(any);
		} else if(mcallCheckType(Map<String,int>)) {
			return Metacall_convertMapToValue<Map<String,int>>(any);
		} else if(mcallCheckType(Map<String,long>)) {
			return Metacall_convertMapToValue<Map<String,long>>(any);
		} else if(mcallCheckType(Map<String,bool>)) {
			return Metacall_convertMapToValue<Map<String,bool>>(any);
		} else if(mcallCheckType(Map<String,void*>)) {
			return Metacall_convertMapToValue<Map<String,void*>>(any);
		} else if(mcallCheckType(Map<String,float>)) {
			return Metacall_convertMapToValue<Map<String,float>>(any);
		} else if(mcallCheckType(Map<String,double>)) {
			return Metacall_convertMapToValue<Map<String,double>>(any);
		} else if(mcallCheckType(Map<String,short>)) {
			return Metacall_convertMapToValue<Map<String,short>>(any);
		} else if(mcallCheckType(Map<String,char>)) {
			return Metacall_convertMapToValue<Map<String,char>>(any);
		} else if(mcallCheckType(Map<String,ArrayList<Any>>)) {
			return Metacall_convertMapToValue<Map<String,ArrayList<Any>>>(any);
		} else if(mcallCheckType(Map<String,Map<String,Any>>)) {
			return Metacall_convertMapToValue<Map<String,Map<String,Any>>>(any);
		}
		else if(mcallCheckType(Map<int,String>)) {
			return Metacall_convertMapToValue<Map<int,String>>(any);
		} else if(mcallCheckType(Map<int,int>)) {
			return Metacall_convertMapToValue<Map<int,int>>(any);
		} else if(mcallCheckType(Map<int,long>)) {
			return Metacall_convertMapToValue<Map<int,long>>(any);
		} else if(mcallCheckType(Map<int,bool>)) {
			return Metacall_convertMapToValue<Map<int,bool>>(any);
		} else if(mcallCheckType(Map<int,void*>)) {
			return Metacall_convertMapToValue<Map<int,void*>>(any);
		} else if(mcallCheckType(Map<int,float>)) {
			return Metacall_convertMapToValue<Map<int,float>>(any);
		} else if(mcallCheckType(Map<int,double>)) {
			return Metacall_convertMapToValue<Map<int,double>>(any);
		} else if(mcallCheckType(Map<int,short>)) {
			return Metacall_convertMapToValue<Map<int,short>>(any);
		} else if(mcallCheckType(Map<int,char>)) {
			return Metacall_convertMapToValue<Map<int,char>>(any);
		} else if(mcallCheckType(Map<int,ArrayList<Any>>)) {
			return Metacall_convertMapToValue<Map<int,ArrayList<Any>>>(any);
		} else if(mcallCheckType(Map<int,Map<String,Any>>)) {
			return Metacall_convertMapToValue<Map<int,Map<String,Any>>>(any);
		}
		else if(mcallCheckType(Map<long,String>)) {
			return Metacall_convertMapToValue<Map<long,String>>(any);
		} else if(mcallCheckType(Map<long,int>)) {
			return Metacall_convertMapToValue<Map<long,int>>(any);
		} else if(mcallCheckType(Map<long,long>)) {
			return Metacall_convertMapToValue<Map<long,long>>(any);
		} else if(mcallCheckType(Map<long,bool>)) {
			return Metacall_convertMapToValue<Map<long,bool>>(any);
		} else if(mcallCheckType(Map<long,void*>)) {
			return Metacall_convertMapToValue<Map<long,void*>>(any);
		} else if(mcallCheckType(Map<long,float>)) {
			return Metacall_convertMapToValue<Map<long,float>>(any);
		} else if(mcallCheckType(Map<long,double>)) {
			return Metacall_convertMapToValue<Map<long,double>>(any);
		} else if(mcallCheckType(Map<long,short>)) {
			return Metacall_convertMapToValue<Map<long,short>>(any);
		} else if(mcallCheckType(Map<long,char>)) {
			return Metacall_convertMapToValue<Map<long,char>>(any);
		} else if(mcallCheckType(Map<long,ArrayList<Any>>)) {
			return Metacall_convertMapToValue<Map<long,ArrayList<Any>>>(any);
		} else if(mcallCheckType(Map<long,Map<String,Any>>)) {
			return Metacall_convertMapToValue<Map<long,Map<String,Any>>>(any);
		}
		throw std::runtime_error("No matching conversion for any with type name "+std::string(any.type().name()));
	}
	
	std::any Metacall::fromMetacallValue(void* value) {
		if(value == nullptr) {
			return Any();
		}
		auto mcallType = metacall_value_id(value);
		switch(mcallType) {
			case METACALL_INT:
				return metacall_value_to_int(value);
			case METACALL_LONG:
				return metacall_value_to_long(value);
			case METACALL_BOOL:
				return (bool)metacall_value_to_bool(value);
			case METACALL_STRING:
				return String(metacall_value_to_string(value));
			case METACALL_FLOAT:
				return metacall_value_to_float(value);
			case METACALL_DOUBLE:
				return metacall_value_to_double(value);
			case METACALL_CHAR:
				return metacall_value_to_char(value);
			case METACALL_SHORT:
				return metacall_value_to_short(value);
			case METACALL_PTR:
				return value;
			case METACALL_BUFFER:
				return value;
			case METACALL_NULL:
				return nullptr;
			case METACALL_ARRAY: {
				auto values = metacall_value_to_array(value);
				size_t size = metacall_value_count(value);
				ArrayList<Any> array;
				array.reserve(size);
				for(size_t i=0; i<size; i++) {
					array.push_back(fromMetacallValue(values[i]));
				}
				return array;
			}
			case METACALL_MAP: {
				auto values = (void***)metacall_value_to_map(value);
				size_t size = metacall_value_count(value);
				for(size_t i=0; i<size; i++) {
					if(metacall_value_id(values[i][0]) != METACALL_STRING) {
						throw std::runtime_error("Unable to convert map with non-string keys");
					}
				}
				Map<String,Any> map;
				for(size_t i=0; i<size; i++) {
					map.insert_or_assign(metacall_value_to_string(values[i][0]), fromMetacallValue(values[i][1]));
				}
				return map;
			}
			default:
				// Invalid metacall_value_id
				assert(false);
		}
	}
	
	void Metacall::destroyMetacallValue(void* value) {
		if(value == nullptr) {
			return;
		}
		metacall_value_destroy(value);
	}
	
	
	
	
	std::any Metacall::call(const String& func, const std::vector<std::any>& args) {
		size_t arg_count = args.size();
		auto args_ptr = std::make_unique<void*[]>(arg_count);
		for(size_t i=0; i<arg_count; i++) {
			args_ptr[i] = toMetacallValue(args[i]);
		}
		void* result_ptr = metacallv(func.data(), args_ptr.get());
		for(size_t i=0; i<arg_count; i++) {
			destroyMetacallValue(args_ptr[i]);
		}
		args_ptr.reset();
		auto result = fromMetacallValue(result_ptr);
		destroyMetacallValue(result_ptr);
		return result;
	}
}
