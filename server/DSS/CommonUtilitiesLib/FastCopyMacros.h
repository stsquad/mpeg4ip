#ifndef __FastCopyMacros__
#define __FastCopyMacros__

#define 	COPY_BYTE( dest, src ) ( *((char*)(dest)) = *((char*)(src)) )
#define 	COPY_WORD( dest, src ) ( *((SInt16*)(dest)) =  *((SInt16*)(src)) )
#define 	COPY_LONG_WORD( dest, src ) ( *((SInt32*)(dest)) =  *((SInt32*)(src)) )
#define 	COPY_LONG_LONG_WORD( dest, src ) ( *((SInt64*)(dest)) =  *((SInt64**)(src)) )

#define 	MOVE_BYTE( dest, src ) ( dest = *((char*)(src)) )
#define 	MOVE_WORD( dest, src ) ( dest =  *((SInt16*)(src)) )
#define 	MOVE_LONG_WORD( dest, src ) ( dest =  *((SInt32*)(src)) )
#define 	MOVE_LONG_LONG_WORD( dest, src ) ( dest =  *((SInt64**)(src)) )


#endif