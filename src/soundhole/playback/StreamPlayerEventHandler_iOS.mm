//
//  StreamPlayerEventHandler_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 1/12/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "StreamPlayerEventHandler_iOS.hpp"
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)

@implementation StreamPlayerEventHandler

-(void)playerItemDidReachEnd:(NSNotification*)notification {
	AVPlayerItem* playerItem = notification.object;
	if(_onItemFinish != nil) {
		_onItemFinish(playerItem);
	}
}

-(void)observeValueForKeyPath:(NSString*)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id>*)change context:(void*)context {
	if([keyPath isEqualToString:@"timeControlStatus"]) {
		AVPlayerTimeControlStatus timeControlStatus = (AVPlayerTimeControlStatus)((NSNumber*)change[NSKeyValueChangeNewKey]).intValue;
		AVPlayerTimeControlStatus prevTimeControlStatus = (AVPlayerTimeControlStatus)((NSNumber*)change[NSKeyValueChangeOldKey]).intValue;
		if(timeControlStatus == prevTimeControlStatus) {
			return;
		}
		if(timeControlStatus == AVPlayerTimeControlStatusPaused) {
			if(_onPause != nil) {
				_onPause();
			}
		} else if(timeControlStatus == AVPlayerTimeControlStatusPlaying) {
			if(prevTimeControlStatus == AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate) {
				if(_onContinue != nil) {
					_onContinue();
				}
			} else {
				if(_onPlay != nil) {
					_onPlay();
				}
			}
		} else if(timeControlStatus == AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate) {
			if(prevTimeControlStatus == AVPlayerTimeControlStatusPaused) {
				if(_onPlay != nil) {
					_onPlay();
				}
			}
			if(_onBuffering != nil) {
				_onBuffering();
			}
		}
	}
}

@end

#endif
