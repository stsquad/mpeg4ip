#import <Cocoa/Cocoa.h>
#import <player_session_wrap.h>

@interface MP4PlayerController : NSObject
{
    CPS* player;
    CMQ* master_queue;
    SDL_sem *master_sem;
	
    NSString* filename;
	
    enum {
        CLOSED,
        PLAYING,
        NOT_PLAYING
    } state;
    BOOL syncThreadStarted;
    
    int screenSize;
    BOOL needToSeek;
	
    IBOutlet NSTextField* firstLine;
    IBOutlet NSTextField* secondLine;
	
    IBOutlet NSButton* pauseButton;
    IBOutlet NSButton* playButton;
    IBOutlet NSButton* stopButton;
    
    IBOutlet NSButton* looping;
    IBOutlet NSPopUpButton* videoSize;
	IBOutlet NSComboBox* urlComboBox;
	IBOutlet NSPanel* openURLPanel;
    
    IBOutlet NSSlider* time;
    IBOutlet NSSlider* volume;
}

- (id)init;
- (IBAction)adjustVolume:(id)sender;
- (IBAction)seek:(id)sender;

- (IBAction)open:(id)sender;

- (IBAction)pause:(id)sender;
- (IBAction)play:(id)sender;
- (IBAction)stop:(id)sender;

- (IBAction)openURL:(id)sender;
- (IBAction)close:(id)sender;
- (IBAction)setVideoSize:(id)sender;
- (IBAction)setLooping:(id)sender;
- (IBAction)doOpenURL:(id)sender;

- (void)stateHasChanged;
- (void)doOpen;
@end
