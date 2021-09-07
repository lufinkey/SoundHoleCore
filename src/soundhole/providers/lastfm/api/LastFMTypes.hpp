//
//  LastFMTypes.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	struct LastFMAPICredentials {
		String key;
		String secret;
	};


	struct LastFMSession {
		String name;
		bool subscriber;
		String key;
		
		static LastFMSession fromJson(const Json& json);
		Json toJson() const;
	};


	struct LastFMWebAuthResult {
		String username;
		String password;
		ArrayList<String> cookies;
	};


	struct LastFMScrobbleRequest {
		struct Item {
			String track;
			String artist;
			String album;
			String albumArtist;
			Date timestamp;
			Optional<bool> chosenByUser;
			Optional<size_t> trackNumber;
			String mbid;
			Optional<double> duration;
			
			void appendQueryItems(std::map<String,String>& queryItems, size_t index) const;
		};
		
		ArrayList<Item> items;
		
		std::map<String,String> toQueryItems() const;
	};


	struct LastFMScrobbleResponse {
		struct IgnoredMessage {
			String code;
			String text;
			
			static IgnoredMessage fromJson(const Json&);
		};
		
		struct Item {
			struct Property {
				Optional<String> text;
				bool corrected;
				
				static Property fromJson(const Json&);
			};
			
			IgnoredMessage ignoredMessage;
			
			Property track;
			Property artist;
			Property album;
			Property albumArtist;
			Date timestamp;
			
			static Item fromJson(const Json&);
			static ArrayList<Item> arrayFromJson(const Json&);
		};
		
		size_t accepted;
		size_t ignored;
		
		ArrayList<Item> scrobbles;
		
		static LastFMScrobbleResponse fromJson(const Json&);
	};
}
