MULTIOUTPUT MPEG4IP
-----------------------

Author : Cesar Gonzalez


Intro
-----

This patch enables mp4live to encode and transmit the same event at different bitrates, window sizes, etc. This is especially usefull if you want to reach differents audiences (ej. broadband users, modem, etc).

Let's suppose you want to transmit a live event at 640x480 900 Kbits, 320x240 150 Kbits and 160x120 at 41 Kbits. Until the patch you need 3 video and 3 audio cards.  If you use this patch you will need only 1 video and 1 audio cards.


Install
-------

FFmpeg support in mp4live is a great improvement. Download and install ffmpeg-0.4.8 and enable it in mpeg4ip (note that you must execute cvs_bootstrap not bootstrap).

./cvs_bootstrap --enable-ffpmeg=/dir/ffmpeg-0.4.8

make

make install


How it works
------------

Here are an asccii graph ilustrating how mp4live will produce the streams we talk about in the intro part :

				---------------------------------
---Frames from Capture cards--->|Main Process, 640x480 864 Kbits |
				---------------------------------
						|
						|			----------------------------------------
						|---Fwd Frames--------->|Attached Process #1 320x240 150 Kbits |
						|			----------------------------------------
						|
						|			----------------------------------------
						|---Fwd Frames--------->|Attached Process #2 160x120 41 Kbits  |
									----------------------------------------

There is one main process that take video and audio frames from the capture devices and forward them to the rest of the processes. Each attached process make its resize/resample work and record the mp4 or transmit the frames via rtp if needed.

There are new directives and new values in the configuration file :

*New configuration directives:

	- cascadeEnable=0/1 This make the process to forward the captured frames to the attached processes.

	- cascadeFile1=file You can give here the configuration file for the first attached process.

	- cascadeFile2=file You can give here the configuration file for the second attached process.

	- cascadeFile3=file You can give here the configuration file for the third attached process.

	- cascadeFile4=file You can give here the configuration file for the fourth attached process.

	- cascadeFile5=file You can give here the configuration file for the fiveth attached process.

* New Values in existing directives:

	- videoSourceType=self You especify "self" when the video frames comes from the main process instead a capture card.

	- audioSourceType=self You especify "self" when the audio frames comes from the main process instead a capture card.


Normally you should use the new cascade* directives in the main process and the new values (videoSourceType=self and audioSourceType=self) in the attached processes.

Example of use
--------------

Lets configure the example we talk about in the intro :


We will need 3 configuration files, we will name them main_rc, att1_rc and att2_rc.

main_rc : 640x480 MPEG-4 (ffmpeg) 800 Kbits / 44100 AAC 64 Kbits
att1_rc : 320x240 MPEG-4 (ffmpeg) 115 Kbits / 16000 AAC 16 Kbits
att2_rc : 160x120 MPEG-4 (ffmpeg) 25 Kbits / 16000 AAC 16 Kbits


Here are the files :

------begin main_rc---------------------------------------------
duration=4
durationUnits=3600
recordEnable=1
rtpEnable=1
audioEnable=1
audioSourceType=oss
audioDevice=/dev/dsp0
audioChannels=1
audioSampleRate=44100
audioBitRateBps=64000
audioEncoding=AAC
audioEncoder=faac
videoEnable=1
videoPreview=0
videoDevice=/dev/video0
videoEncoder=ffmpeg
videoInput=0
videoSignal=0
videoTuner=0
videoChannelListIndex=5
videoChannelIndex=74
videoRawWidth=640
videoRawHeight=480
videoAspectRatio=1.34
videoFrameRate=20
videoBitRate=800
videoBrightness=50
videoHue=50
videoColor=50
videoContrast=50
recordMp4Optimize=1
rtpDestAddress=127.0.0.1
rtpAudioDestPort=2048
rtpVideoDestPort=2050
rtpMulticastTtl=127
sdpFile=main.sdp
recordMP4file=main.mp4
cascadeEnable=1
cascadeFile1=att1_rc
cascadeFile2=att2_rc
-------end main_rc---------------------------------------------

------begin att1_rc--------------------------------------------
recordEnable=1
rtpEnable=1
signalHalt=hup
audioSourceType=self
audioEnable=1
audioChannels=1
audioSampleRate=16000
audioBitRateBps=16000
audioEncoding=AAC
audioEncoder=faac
videoSourceType=self
videoPreview=0
videoEncodedPreview=false
videoEnable=1
videoEncoder=ffmpeg
videoInput=1
videoSignal=0
videoTuner=0
videoChannelListIndex=5
videoChannelIndex=74
videoRawWidth=320
videoRawHeight=240
videoAspectRatio=1.34
videoFrameRate=10
videoBitRate=110
videoBrightness=50
videoHue=50
videoColor=50
videoContrast=50
recordMp4Optimize=1
rtpDestAddress=127.0.0.1
rtpAudioDestPort=2036
rtpVideoDestPort=2034
rtpMulticastTtl=127
sdpFile=att1.sdp
recordMP4file=att1.mp4
-------end att1_rc---------------------------------------------

------begin att2_rc--------------------------------------------
recordEnable=1
rtpEnable=1
signalHalt=hup
audioSourceType=self
audioEnable=1
audioChannels=1
audioSampleRate=16000
audioBitRateBps=16000
audioEncoding=AAC
audioEncoder=faac
videoSourceType=self
videoPreview=0
videoEnable=1
videoEncoder=ffmpeg
videoInput=1
videoSignal=0
videoTuner=0
videoChannelListIndex=5
videoChannelIndex=74
videoRawWidth=288
videoRawHeight=216
videoFrameRate=5
videoBitRate=25
videoBrightness=50
videoHue=50
videoColor=50
videoContrast=50
recordMp4Optimize=1
rtpDestAddress=127.0.0.1
rtpAudioDestPort=2030
rtpVideoDestPort=2032
rtpMulticastTtl=127
sdpFile=att2.sdp
recordMP4file=att2.mp4
-------end att2_rc---------------------------------------------


So we have the configuration files in the same directory :

#ls
att1_rc  att2_rc  main_rc

We launch mp4live against the main_rc file mp4live will read the location of attached processes configuration files and init them :

#mp4live -f main_rc -h
00:51:28.316-mp4live-3: type 0 Television type 1
00:51:28.317-mp4live-3: Has tuner
00:51:28.317-mp4live-3: type 1 Composite1 type 2
00:51:28.317-mp4live-3: type 2 S-Video type 2
00:51:28.317-mp4live-3: NOTE: You do not have the latest version of OSS with SNDCTL_DSP_GETERROR
00:51:28.317-mp4live-3: This could have long term sync issues
00:51:28.317-mp4live-3: Video statistics surpass ASP Level 5 - bit rate 800000 MpbSec 24000
00:51:28.325-mp4live-3: mp4live version 1.0.11 V4L2

00:51:28.343-rtp-6: Created database entry for ssrc 0x79096aea (1 valid sources)
00:51:28.344-rtp-6: Created database entry for ssrc 0x30b5de97 (1 valid sources)
00:51:28.396-rtp-6: Created database entry for ssrc 0x7821f302 (1 valid sources)
00:51:28.397-rtp-6: Created database entry for ssrc 0x4accc986 (1 valid sources)
00:51:28.447-rtp-6: Created database entry for ssrc 0x2f12dce0 (1 valid sources)
00:51:28.448-rtp-6: Created database entry for ssrc 0x58cfeaa4 (1 valid sources)
00:51:28.536-mp4live-3: Video statistics surpass ASP Level 5 - bit rate 800000 MpbSec 24000

[Ctrl-C]

00:51:58.568-mp4live-3: Received stop signal 2
00:51:59.533-rtp-6: Dropped membership of multicast group
00:51:59.533-rtp-6: Dropped membership of multicast group
00:51:59.533-rtp-6: Dropped membership of multicast group
00:51:59.533-rtp-6: Dropped membership of multicast group
00:51:59.592-rtp-6: Dropped membership of multicast group
00:51:59.592-rtp-6: Dropped membership of multicast group
00:51:59.593-rtp-6: Dropped membership of multicast group
00:51:59.593-rtp-6: Dropped membership of multicast group
00:52:00.154-rtp-6: Dropped membership of multicast group
00:52:00.154-rtp-6: Dropped membership of multicast group
00:52:00.154-rtp-6: Dropped membership of multicast group
00:52:00.155-rtp-6: Dropped membership of multicast group


#mp4info *.mp4
mp4info version 1.0.11
att1.mp4:
Track   Type    Info
1       video   MPEG-4 Simple @ L2, 30.912 secs, 142 kbps, 320x240 @ 9.90 fps
2       audio   MPEG-4 AAC LC, 30.720 secs, 16 kbps, 16000 Hz
3       hint    Payload MP4V-ES for track 1
4       hint    Payload mpeg4-generic for track 2
5       od      Object Descriptors
6       scene   BIFS
Metadata Tool: mp4live version 1.0.11 V4L2
att2.mp4:
Track   Type    Info
1       video   MPEG-4 Simple @ L0, 31.012 secs, 52 kbps, 288x216 @ 4.93 fps
2       audio   MPEG-4 AAC LC, 30.720 secs, 16 kbps, 16000 Hz
3       hint    Payload MP4V-ES for track 1
4       hint    Payload mpeg4-generic for track 2
5       od      Object Descriptors
6       scene   BIFS
Metadata Tool: mp4live version 1.0.11 V4L2
main.mp4:
Track   Type    Info
1       video   MPEG-4 Adv Simple@L4, 30.904 secs, 2944 kbps, 640x480 @ 11.33 fps
2       audio   MPEG-4 AAC LC, 30.882 secs, 64 kbps, 44100 Hz
3       hint    Payload MP4V-ES for track 1
4       hint    Payload mpeg4-generic for track 2
5       od      Object Descriptors
6       scene   BIFS
Metadata Tool: mp4live version 1.0.11 V4L2


Limitations
-----------

* You can have a maximum of 5 attached processes. 1 main process + 5 attached processes = 6 differents streams.

* You must take special attention on chossing the main process because it will feed the others. Imagine you want two streams : 640x480 25 frames/seg and 320x240 10 frames/seg, if you choose the second one as main process, you will never get 25 frames/seg in the first one.

* You can especify differents sizes in the attached proccesses but it you must keep the relation between the width and height of the main and attached processes. For example if your main process is 320x240 you can not have an attached process with 172x144.

Contact
-------

* If you have some question you can reach me at cesar [at] eureka-sistemas.com
