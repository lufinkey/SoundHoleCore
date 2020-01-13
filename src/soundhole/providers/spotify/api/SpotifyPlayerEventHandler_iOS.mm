//
//  SpotifyPlayerEventHandler_iOS.mm
//  SoundHoleCore-iOS
//
//  Created by Luis Finke on 9/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyPlayerEventHandler_iOS.hpp"
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)

@implementation SpotifyPlayerEventHandler

-(void)audioStreaming:(SPTAudioStreamingController*)audioStreaming didReceiveError:(NSError*)error {
	if(_onError != nil) {
		_onError(error);
	}
}

-(void)audioStreamingDidLogin:(SPTAudioStreamingController*)audioStreaming {
	if(_onLogin != nil) {
		_onLogin();
	}
}

-(void)audioStreamingDidLogout:(SPTAudioStreamingController*)audioStreaming {
	if(_onLogout != nil) {
		_onLogout();
	}
}

-(void)audioStreamingDidEncounterTemporaryConnectionError:(SPTAudioStreamingController*)audioStreaming {
	if(_onTemporaryConnectionError != nil) {
		_onTemporaryConnectionError();
	}
}

-(void)audioStreaming:(SPTAudioStreamingController*)audioStreaming didReceiveMessage:(NSString*)message {
	if(_onMessage != nil) {
		_onMessage(message);
	}
}

-(void)audioStreamingDidDisconnect:(SPTAudioStreamingController*)audioStreaming {
	if(_onDisconnect != nil) {
		_onDisconnect();
	}
}

-(void)audioStreamingDidReconnect:(SPTAudioStreamingController*)audioStreaming {
	if(_onReconnect != nil) {
		_onReconnect();
	}
}

-(void)audioStreaming:(SPTAudioStreamingController*)audioStreaming didReceivePlaybackEvent:(SpPlaybackEvent)event {
	if(_onPlaybackEvent != nil) {
		_onPlaybackEvent(event);
	}
}

-(void)audioStreaming:(SPTAudioStreamingController*)audioStreaming didChangePlaybackStatus:(BOOL)isPlaying {
	if(_onChangePlaybackStatus != nil) {
		_onChangePlaybackStatus(isPlaying);
	}
}

@end

#endif
