
#ifndef __pathdelimiter__
#define __pathdelimiter__


#if __MACOS__
	#define	kPathDelimiterString ":"
	#define	kPathDelimiterChar ':'
	#define kPartialPathBeginsWithDelimiter 1
#else
	#define	kPathDelimiterString "/"
	#define	kPathDelimiterChar '/'
	#define kPartialPathBeginsWithDelimiter 0
#endif


#endif
