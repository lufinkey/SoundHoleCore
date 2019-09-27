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
	_onError(error);
}

-(void)audioStreamingDidLogin:(SPTAudioStreamingController*)audioStreaming {
	_onLogin();
}

-(void)audioStreamingDidLogout:(SPTAudioStreamingController*)audioStreaming {
	_onLogout();
}

-(void)audioStreamingDidEncounterTemporaryConnectionError:(SPTAudioStreamingController*)audioStreaming {
	_onTemporaryConnectionError();
}

-(void)audioStreaming:(SPTAudioStreamingController*)audioStreaming didReceiveMessage:(NSString*)message {
	_onMessage(message);
}

-(void)audioStreamingDidDisconnect:(SPTAudioStreamingController*)audioStreaming {
	_onDisconnect();
}

-(void)audioStreamingDidReconnect:(SPTAudioStreamingController*)audioStreaming {
	_onReconnect();
}

-(void)audioStreaming:(SPTAudioStreamingController*)audioStreaming didReceivePlaybackEvent:(SpPlaybackEvent)event {
	_onPlaybackEvent(event);
}

@end

#endif
