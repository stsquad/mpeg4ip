#import "MP4PlayerController.h"
#import <Foundation/NSTimer.h>

/*
 * Objective-C++ controller object to interface with the mpeg4ip CPlayerSession model object.
 *
 * We have to wrap the C++ CPlayerSession object in a set of C-callable functions
 * (see player_session_wrap.h) because Objective-C++ is nowhere to be found these
 * days.  It would be much cooler if we could just use it directly.
 */
 
@implementation MP4PlayerController

- (id)init
{
    self = [super init];
    player = 0;
    state = CLOSED;
	// XXX: This stuff doesn't actually work.  I need to initialize my stuff after
	// de-archiving, but in some other method... what's it called?
    return self;
}

- (void)stateHasChanged
{
    if (state == CLOSED) {
        [volume setEnabled:NO];
        [time setEnabled:NO];
        [playButton setEnabled:NO];
        [pauseButton setEnabled:NO];
        [stopButton setEnabled:NO];
        [firstLine setObjectValue:@"mpeg4ip"];
        [secondLine setObjectValue:@"version 0.7"];
    }
    else {
        [firstLine setObjectValue:filename];
        [volume setEnabled:YES];
        [time setEnabled:YES];
        if ((CPS_get_session_state(player) != SESSION_BUFFERING) && (!needToSeek))
            [time setDoubleValue:CPS_get_playing_time(player)];
        [time setMaxValue:CPS_get_max_time(player)];
        switch (state) {
            case PLAYING:
                [secondLine setObjectValue:@"Playing"];
                [playButton setEnabled:NO];
                [pauseButton setEnabled:YES];
                [stopButton setEnabled:YES];
                break;
            default:
                [playButton setEnabled:YES];
                [pauseButton setEnabled:NO];
				if ([time doubleValue] == 0.0) {
					[stopButton setEnabled:NO];
					[secondLine setObjectValue:@"Stopped"];
				}
				else {
					[stopButton setEnabled:YES];
					[secondLine setObjectValue:@"Paused"];
				}
				break;
        }
    }
}

- (IBAction)adjustVolume:(id)sender
{
    float x = [volume floatValue];
    
    if (player)
        CPS_set_audio_volume(player, x);
}

- (IBAction)seek:(id)sender
{
    NSLog(@"seek: %f", [time doubleValue]);
    
    if (state == PLAYING) {
        assert(player != nil);
        CPS_pause_all_media(player);
        CPS_play_all_media(player, ([time doubleValue] == 0.0),
                            [time doubleValue]);
    }
    else {
        needToSeek = YES;
    }
    [self stateHasChanged];
}

- (IBAction)open:(id)sender
{
    int result;
    NSArray *fileTypes;
    NSOpenPanel *oPanel;

    // Close first if already open
    if (player != 0) {
        [self close:sender];
    }
    
    // Ask user what to open
    NSLog(@"getting file name . . .");
    fileTypes = [NSArray arrayWithObject:@"td"];
    oPanel = [NSOpenPanel openPanel];

    result = [oPanel runModalForDirectory:nil file:nil types:nil];
    if (result != NSOKButton)
        return;
    filename = [oPanel filename];
	[self doOpen];
}

- (IBAction)openURL:(id)sender
{
	[[openURLPanel windowController] showWindow:sender];
}


- (IBAction)doOpenURL:(id)sender
{
	[[openURLPanel windowController] close];
	filename = [urlComboBox objectValue];
	(void) [urlComboBox removeItemWithObjectValue:filename];
	[urlComboBox insertItemWithObjectValue:filename atIndex:0];
	[self doOpen];
}


- (void)doOpen
{
    int result;
    char errbuf[200];

    if (!master_queue)
        master_queue = new_CMQ();
    if (!master_sem)
        master_sem = SDL_CreateSemaphore(0);
        
    player = new_CPS(master_queue, master_sem, "MPEG-4 Player");

    result = parse_name_for_c_session(player, [filename lossyCString], errbuf, sizeof errbuf);
    if (result < 0) {
        NSString* errstr = [NSString stringWithCString:errbuf];
        NSLog(@"Name parse failed: %s", errbuf);        CPS_destroy(player);
        NSRunAlertPanel(@"Cannot open file", errstr, nil, nil, nil);
        return;
    }
    
    /* Cool beans, we have a file open. */
    /* FIXME: Get screen location from this window somehow. */
    CPS_set_screen_location(player, 100, 100);
    CPS_set_screen_size(player, screenSize, 0);
    
    [time setMaxValue:CPS_get_max_time(player)];
    [time setDoubleValue:0];
    
    [self adjustVolume:nil];
    
    if (CPS_session_is_seekable(player))
        [time setEnabled:YES];
    else
        [time setEnabled:NO];
    
    state = NOT_PLAYING;
    syncThreadStarted = NO;
    
    [self stateHasChanged];
}

- (IBAction)pause:(id)sender
{
    session_state_t pst;
    assert(state == PLAYING);
    assert(player != nil);
    pst = CPS_get_session_state(player);
    if (CPS_get_session_state(player) == SESSION_DONE) {
        // Nobody else noticed this yet.
        state = NOT_PLAYING;
        return;
    }
    assert(pst != SESSION_PAUSED);
    CPS_pause_all_media(player);
    state = NOT_PLAYING;
    [self stateHasChanged];
}

- (IBAction)play:(id)sender
{
    session_state_t pst;
    double newtime;
	static BOOL timerStarted = NO;
	
    assert(player != nil);
    assert(state == NOT_PLAYING);
	
	if (!timerStarted) {
		[NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(stateHasChanged) userInfo:nil repeats:YES];
		timerStarted = YES;
	}
	
    pst = CPS_get_session_state(player);
    assert(pst == SESSION_PAUSED || pst == SESSION_DONE);
    if (!syncThreadStarted) {
        NSLog(@"setting up sync thread . . .");
        CPS_set_up_sync_thread(player);
        syncThreadStarted = YES;
    }
    if (needToSeek) {
        NSLog(@"Need to seek");
        newtime = [time doubleValue];
        needToSeek = NO;
    }
    else {
        NSLog(@"Don't need to seek");
        newtime = CPS_get_playing_time(player);
    }
    CPS_play_all_media(player, (newtime == 0.0), newtime);
    state = PLAYING;
    [self stateHasChanged];
}

- (IBAction)stop:(id)sender
{
    [time setDoubleValue:0];
    needToSeek = YES;
    if (CPS_get_session_state(player) == SESSION_DONE)
        state = NOT_PLAYING;
    if (state == PLAYING)
        [self pause:sender];
    else
        [self stateHasChanged];
}


- (IBAction)close:(id)sender
{
    SDL_Event event;
    NSLog(@"Destroying player . . .");
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
    CPS_destroy(player);
    player = 0;
    filename = nil;
    state = CLOSED;
    syncThreadStarted = NO;
    [self stateHasChanged];
    return;
}

- (IBAction)setVideoSize:(id)sender
{
    BOOL setFullScreen = NO;
    
    switch ([sender indexOfSelectedItem]) {
        case 0:			screenSize = 1; break;
        case 1: case -1:	screenSize = 2; break;
        case 2:			screenSize = 4; break;
        case 3:			setFullScreen = YES; break;
    }
    
    NSLog(@"set video size... %d", screenSize);
    
    if (player)
        CPS_set_screen_size(player, screenSize, setFullScreen);

    /* if ((!player) && setFullScreen) do something useful */
}

- (IBAction)setLooping:(id)sender
{
	if ([sender state] == NSOnState) {
		NSRunAlertPanel(@"Error", @"Looping is not yet implemented", nil, nil, nil);
		[sender setState:NSOffState];
	}
    return;
}


@end
