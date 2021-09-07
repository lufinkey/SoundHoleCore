//
//  LastFMAPIRequest.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/6/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/HttpClient.hpp>
#include "LastFMTypes.hpp"

namespace sh {
	class LastFMAPIRequest {
	public:
		String apiRoot;
		String apiMethod;
		utils::HttpMethod httpMethod;
		Map<String,String> params;
		Optional<LastFMAPICredentials> credentials;
		Optional<LastFMSession> session;
		
		utils::HttpRequest toHttpRequest() const;
		Promise<Json> perform() const;
	};
}
